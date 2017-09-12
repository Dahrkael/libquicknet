// Copyright (c) 2017 Santiago Fernandez Ortiz
// 
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <vector>
#include <sstream>
#include "quicknet_peer.h"
#include "quicknet_remotepeer.h"
#include "quicknet_packet.h"
#include "quicknet_messageslookup.h"
#include "quicknet_stream.h"
#include "quicknet_log.h"
#include "quicknet_time.h"

namespace quicknet
{
	// our unique game ID
	static const uint32_t s_gameIdentifier = 0xDEADCAFE;
	// for connection verification
	static const uint32_t s_challengeSeed = 0x123456;

	// server info
	static uint8_t			s_serverPeerID = 0;
	static uint16_t			s_serverPort = 8000;
	// the broadcast address should be always the same
	static Address			s_broadcastAddress = Address("255.255.255.255", s_serverPort);
	static const uint64_t	s_broadcastProbeDelay = 1000;
	// this should be at least the biggest message size (more with merging on)
	static const uint16_t	s_bufferSize = 1400;

	// how much time without receiving reliables/keepalives before dropping
	static uint64_t s_connectionTimeout = 10 * 1000;
	// how much time should we wait for new acks before sending a KeepAlive
	static uint64_t s_maxWithoutAcks = 100;

	// max size before we stop adding messages in a packet
	static const uint16_t s_maximumPacketSize = 1400;

	// how many packets do we send per second at most
	static uint64_t s_sendRate = 20;
	static const uint64_t m_sendTime = 1000 / s_sendRate;

	Peer::Peer(bool serverMode, uint8_t maxPeers)
		: m_maxPeers(maxPeers)
		, m_state(serverMode ? NetPeerState::ServerMode : NetPeerState::Disconnected)
		, m_socket(false, 0)
		, m_assignedID(0xFF)
		, m_peers()
		, m_addressIDs()
		, m_lastSend(0)
		, m_fakePacketLoss(0.0f)
		, m_fakeLatency()
		, m_rng((uint32_t)Utils::GetElapsedMilliseconds())
	{
		// bind if its the server to accept incoming connections
		if (IsServer())
		{
			Log::Info("Peer started on server mode, binding to 0.0.0.0...");
			Address serverAddr("0.0.0.0", s_serverPort);
			m_socket.Bind(serverAddr);
		}

		// allow broadcast for all peers
		m_socket.AllowBroadcast(true);

		m_recvBuffer = new uint8_t[s_bufferSize];
		m_sendBuffer = new uint8_t[s_bufferSize];
	}

	Peer::~Peer()
	{
		m_socket.Close();
		if (m_recvBuffer != nullptr)
		{
			delete[] m_recvBuffer;
		}
		if (m_recvBuffer != nullptr)
		{
			delete[] m_sendBuffer;
		}
	}

	bool Peer::FindServers()
	{
		Log::Info("Finding LAN servers...");

		if (IsServer()) { return false; }

		if (m_state != NetPeerState::Disconnected)
		{
			DisconnectAll();
		}

		m_state = NetPeerState::Searching;
		m_lastSend = Utils::GetElapsedMilliseconds() - 1000;

		return true;
	}

	bool Peer::ConnectTo(const Address& address)
	{
		std::ostringstream ss;
		ss << "Trying to connect to " << address.ToIPv4String();
		Log::Info(ss.str());

		m_state = NetPeerState::Connecting;
		// add the server peer now
		AddPeer(address);

		// send a request message with the correct game identifier
		std::unique_ptr<MessageConnectionRequest> request = MessageConnectionRequest::Create();
		request->m_gameID = s_gameIdentifier;
		return sendMessage(address, std::move(request));
	}

	uint8_t Peer::AddPeer(const Address& address)
	{
		Log::Info("AddPeer called");
		if (IsServer())
		{
			uint8_t assignedID = findFreePeerID();
			if (assignedID != 0xFF)
			{
				RemotePeer* peer = new RemotePeer(address);
				peer->SetSate(NetPeerState::Connecting);
				// assign an ID for the peer
				peer->m_assignedID = assignedID;
				m_peers[assignedID] = peer;
				m_addressIDs[address] = assignedID;
			}

			return assignedID;
		}
		else
		{
			RemotePeer* peer = new RemotePeer(address);
			peer->SetSate(NetPeerState::ServerMode);
			peer->m_assignedID = 0x00;
			m_peers[0] = peer;
			m_addressIDs[address] = 0x00;

			return 0x00;
		}
	}

	bool Peer::DisconnectPeer(uint8_t peerID, uint8_t amount)
	{
		Log::Info("DisconnectPeer called");
		if (!peerExists(peerID)) { return false; }

		// send it 5 times just to be sure it arrives if network condition is not awful
		for (int i = 0; i < amount; i++)
		{
			std::unique_ptr<MessageDisconnectionRequest> message = MessageDisconnectionRequest::Create();
			sendMessage(m_peers[peerID]->Address(), std::move(message));
		}
		// mark it as disconnected to erase it later
		m_peers[peerID]->SetSate(NetPeerState::Disconnected);

		OnDisconnection(peerID);

		return true;
	}

	void Peer::DisconnectAll()
	{
		Log::Info("DisconnectAll called");

		// this is not very efficient, but its a rarely invoked function
		for (std::pair<const uint8_t, RemotePeer*>& peer : m_peers)
		{
			DisconnectPeer(peer.first, 5);
		}

		if (!IsServer())
		{
			m_state = NetPeerState::Disconnected;
			m_assignedID = 0xFF;
		}
	}

	void Peer::UpdateNetwork()
	{
		switch (m_state)
		{
		case NetPeerState::ServerMode:
		{
			// check if we got some answers
			receive();
			// do maintenance stuff on peers
			updatePeers();
			// send pending messages
			send();
		}
		break;
		case NetPeerState::Disconnected:
		{
			// nothing
		}
		break;
		case NetPeerState::Searching:
		{
			// send one probe per second
			if ((Utils::GetElapsedMilliseconds() - m_lastSend) >= s_broadcastProbeDelay)
			{
				Log::Info("Sending discovery probe");
				std::unique_ptr<MessageDiscoveryRequest> message = MessageDiscoveryRequest::Create();
				message->m_gameID = s_gameIdentifier;
				if (!sendMessage(s_broadcastAddress, std::move(message)))
				{
					Log::Error("Sending Discovery Message failed");
				}
				m_lastSend = Utils::GetElapsedMilliseconds();
			}

			// check if we got some answers
			receive();
			// to allow fake latency during search
			updatePeers();
		}
		break;
		case NetPeerState::Connecting:
		{
			// check new messages
			receive();
			// to allow fake latency
			updatePeers();
			// send pending messages
			send();
		}
		break;
		case NetPeerState::Connected:
		{
			// check new messages
			receive();
			// manage connection
			updatePeers();
			// send pending messages
			send();
		}
		break;
		default:
			break;
		}
	}

	bool Peer::SendTo(uint8_t peerID, std::unique_ptr<Message> message)
	{
		// this is O(1) because its an unordered map
		auto peer = m_peers.find(peerID);
		if (peer != m_peers.end())
		{
			return SendTo(peer->second, std::move(message));
		}
		return false;
	}

	bool Peer::SendTo(RemotePeer* peer, std::unique_ptr<Message> message)
	{
		if (peer == nullptr) { return false; }
#if QUICKNET_VERBOSE
		std::ostringstream ss;
		ss << "SendTo: Sending message with ID" << message->m_header.m_messageID << " to peer " << peer->m_assignedID;
		//Log::Info(ss.str());
#endif

	// add the message for later sending
		peer->EnqueueMessage(std::move(message));
		return true;

		// DEBUG: this is to quick check
		//bool success = sendMessage(peer->Address(), std::move(message));
		//return success;
	}

	bool Peer::SendToAll(std::unique_ptr<Message> message)
	{
		// this is not the best way, but a workaround for unique pointers
		for (std::pair<const uint8_t, RemotePeer*>& peer : m_peers)
		{
			if (m_peers.size() > 1)
			{
				message->m_header = message->GenerateHeader();
				std::unique_ptr<Message> copy = GetMessageFromID((MessageIDs)message->m_header.m_messageID);
				message->CopyTo(copy.get());
				SendTo(peer.second, std::move(copy));
			}
			else
			{
				SendTo(peer.second, std::move(message));
			}
		}
		return false;
	}

	void Peer::SetServerMode(bool enable)
	{
		std::ostringstream ss;
		ss << "Setting server mode to " << enable;
		Log::Info(ss.str());

		// disconnect everything to avoid potential issues
		if (m_state != NetPeerState::Disconnected)
		{
			DisconnectAll();
		}

		// set the correct state
		m_state = enable ? NetPeerState::ServerMode : NetPeerState::Disconnected;
		// FIXME: somehow broadcast stops working if uncomment this
		// get a new socket to avoid binding issues
		//m_socket.Close();
		//m_socket = UDPSocket(false, 0);
		//m_socket.AllowBroadcast(true);
		if (IsServer())
		{
			Log::Info("Server mode started, binding to 0.0.0.0...");
			Address serverAddr("0.0.0.0", s_serverPort);
			m_socket.Bind(serverAddr);
		}
	}

	void Peer::SetFakePacketLoss(float percentage)
	{
		m_fakePacketLoss = ((percentage < 0.0f) ? 0.0f : (percentage > 1.0f ? 1.0f : percentage));

		std::ostringstream ss;
		ss << "Fake Packet Loss set to: " << m_fakePacketLoss;
		Log::Info(ss.str());
	}

	void Peer::SetFakeLatency(uint32_t milliseconds)
	{
		m_fakeLatency.SetLatency(milliseconds);

		std::ostringstream ss;
		ss << "Fake latency set to " << milliseconds << "ms";
		Log::Info(ss.str());
	}

	const uint32_t Peer::RTT()
	{
		if (m_state == NetPeerState::Connected)
		{
			if (!IsServer())
			{
				return m_peers[0]->RTT();
			}
			else
			{
				uint32_t avg = 0;
				for (std::pair<const uint8_t, RemotePeer*>& peerPair : m_peers)
				{
					avg += peerPair.second->RTT();
				}
				avg /= m_peers.size();
				return avg;
			}
		}
		return 0;
	}

	uint8_t Peer::findFreePeerID()
	{
		// 0 is the server
		for (int i = 1; i < m_maxPeers + 1; i++)
		{
			if (m_peers.find(i) == m_peers.end())
			{
				return i;
			}
		}

		Log::Warn("No more free Peer IDs available");
		return 0xFF;
	}

	bool Peer::peerExists(uint8_t peerID)
	{
		return (m_peers.find(peerID) != m_peers.end());
	}

	void Peer::updatePeers()
	{
		// retrieve ready to process messages from fake latency
		{
			std::unique_ptr<Message> message = nullptr;
			Address address;

			m_fakeLatency.GetMessageReady(message, address);
			while (message)
			{
#if QUICKNET_VERBOSE
				std::ostringstream ss;
				ss << "LatencyFaker gave us a " << message->Name() << " with sequence " << message->m_header.m_sequence;
				Log::Info(ss.str());
#endif
				RemotePeer* peer = addressToPeer(address);
				if (peer == nullptr)
				{
					RemotePeer unk(address);
					processMessage(message.get(), &unk);
				}
				else
				{
					processMessage(message.get(), peer);
				}

				// get another until we run out of them
				m_fakeLatency.GetMessageReady(message, address);
			}
		}

		// vector to keep the IDs from disconnected peers that need to be removed
		std::vector<uint8_t> toRemove;

		for (std::pair<const uint8_t, RemotePeer*>& peerPair : m_peers)
		{
			RemotePeer* peer = peerPair.second;

			// if we dont get any message in a long time, disconnect
			if (peer->MillisecondsSinceLastMessage() > s_connectionTimeout)
			{
				DisconnectPeer(peer->m_assignedID);
			}

			// keep the ID to remove it if needed
			if (peer->State() == NetPeerState::Disconnected)
			{
#if QUICKNET_VERBOSE
				std::ostringstream ss;
				ss << "Adding peer" << (uint32_t)peer->m_assignedID << " for removal";
				Log::Debug(ss.str());
#endif
				toRemove.push_back(peer->m_assignedID);
				continue;
			}

			// if we got no acks in s_maxWithoutAck seconds
			if (peer->MillisecondsSinceLastAck() > s_maxWithoutAcks)
			{
#if QUICKNET_VERBOSE
				Log::Info("Sending KeepAlive because connection inactivity");
#endif
				// send an unreliable keepAlive (which will be returned, hopefully)
				std::unique_ptr<MessageKeepAlive> keepAlive = MessageKeepAlive::Create();
				keepAlive->m_timeStamp = Utils::GetElapsedMilliseconds();
				keepAlive->m_serverSent = IsServer() ? 0x01 : 0x00;
				SendTo(peer, std::move(keepAlive));
				// we will send another in s_maxWithoutAcks if we still get nothing
				peer->UpdateLastAckTime();
			}
		}

		if (!toRemove.empty())
		{
			// remove the peers from the lists safely
			for (const uint8_t peerID : toRemove)
			{
				RemotePeer* peer = m_peers[peerID];
				m_addressIDs.erase(peer->Address());
				m_peers.erase(peerID);
				delete peer;
			}
		}
	}

	void Peer::receive()
	{
		if (m_socket.IsValid())
		{
			// TODO: clear the recv buffer before? (performance penalty)
			// since I use fixed sizes I don't see an issue not clearing it

			// get the incoming data from the socket
			Address remote;
			uint32_t read;
			bool success;

			do
			{
				read = 0;
				success = m_socket.Recv(m_recvBuffer, s_bufferSize, &read, remote);
				if (success && read > 0)
				{
					// if packet loss is active, discard the buffer directly
					if ((m_fakePacketLoss > 0.0f) && (m_rng.GetFloat() <= m_fakePacketLoss))
					{
						Log::Info("receive: Fake Packet Loss kicked in!");
						break;
					}

					// check if we have a peer from this address
					RemotePeer* peer = addressToPeer(remote);
					if (peer == nullptr)
					{
#if QUICKNET_VERBOSE
						std::ostringstream ss;
						ss << "Received " << read << " bytes from " << remote.ToIPv4String() << "(peer unknown)";
						Log::Info(ss.str());
#endif
						// if we have no entry, create a temporary one
						RemotePeer unknownPeer(remote);
						// parse the packets inside the buffer
						parseBuffer(read, &unknownPeer);
					}
					else
					{
#if QUICKNET_VERBOSE
						std::ostringstream ss;
						ss << "Received " << read << " bytes from " << remote.ToIPv4String() << "(peer " << (uint32_t)peer->m_assignedID << ")";
						Log::Info(ss.str());
#endif
						// parse the packets inside the buffer
						parseBuffer(read, peer);
					}
				}
			} while (success && read > 0);
		}
	}

	void Peer::parseBuffer(uint32_t length, RemotePeer* peer)
	{
		if (length < PacketHeader::Size())
		{
			Log::Warn("Received data is smaller than packet header size");
			return;
		}

		Stream stream(m_recvBuffer, length, NetStreamMode::Read);

		// read the packet header first
		PacketHeader packetHeader;
		packetHeader.FromStream(stream);

		// discard the whole packet now if its wrong to save time
		if (!packetHeader.IsChecksumValid(m_recvBuffer, length))
		{
			Log::Warn("Packet checksum is invalid. Discarding...");
			return;
		}

		// check acknowledgments here
		processAcks(peer, packetHeader);

		// cycle through all the messages contained in this packet
		while (!stream.Full())
		{
			// read message header first, this way we can skip the message earlier
			MessageHeader header;
			header.FromStream(stream);

			// check sequence and discard if necessary
			if (peer->State() != NetPeerState::Disconnected && !header.IsUnsequenced())
			{
				// if sequence is newer, update it
				if (peer->IsSequenceNewer(header.m_sequence, peer->CurrentSequenceIn()))
				{
#if QUICKNET_VERBOSE
					std::ostringstream ss;
					ss << "Received sequence is newer (" << header.m_sequence << " vs " << peer->CurrentSequenceIn() << ")";
					Log::Info(ss.str());
#endif
					peer->SetSequenceIn(header.m_sequence);
					// save it for tracking
					peer->SaveReceivedSequence(header.m_sequence, true);
				}
				else
				{
#if QUICKNET_VERBOSE
					std::ostringstream ss;
					ss << "Received sequence is older (" << header.m_sequence << " vs " << peer->CurrentSequenceIn() << ")";
					Log::Info(ss.str());
#endif
					// check flags so we dont discard the important ones
					if (header.IsOrdered() && !header.IsReliable())
					{
#if QUICKNET_VERBOSE
						Log::Info("Skipping message because old sequence");
#endif
						// skip this message
						stream.Skip(header.m_size);
						continue;
					}

					// check if its duplicated
					if (peer->MessageDuplicated(header.m_sequence))
					{
						// skip this message
						stream.Skip(header.m_size);
						continue;
					}
					else
					{
						// save it for tracking
						peer->SaveReceivedSequence(header.m_sequence, false);
					}
				}
			}

			// get a message instance from the ID
			std::unique_ptr<Message> message = GetMessageFromID((MessageIDs)header.m_messageID);
			if (message)
			{
				bool success = message->FromStream(stream);
				if (success)
				{
					message->m_header = header;

					// keep the message waiting if we have fake latency
					if (m_fakeLatency.CurrentLatency() > 0)
					{
#if QUICKNET_VERBOSE
						Log::Info("Fake Latency active: saving message for later");
#endif
						m_fakeLatency.AddPendingMessage(std::move(message), peer);
					}
					else
					{
						// do the actual processing
						processMessage(message.get(), peer);

						// unknown peers should send 1 message per packet
						// known peers that just disconnected need no further processing
						if (peer->State() == NetPeerState::Disconnected)
						{
							return;
						}
					}
				}
				else
				{
					Log::Warn("Message failed to deserialize. Skipping");
					stream.Skip(header.m_size);
					continue;
				}
			}
			else
			{
				// message ID is invalid
				Log::Warn("Received Message ID is invalid. Skipping");
				// in this case, skip the message
				stream.Skip(header.m_size);
				continue;
			}
		}
	}

	void Peer::processMessage(const Message* const message, RemotePeer* peer)
	{
		// we got a new message, so update the connection timeout
		peer->UpdateLastMessageTime();

		// process the message here if its a management message
		// if not, pass it to the callback
		if (message->m_header.IsSystem())
		{
			MessageIDs id = (MessageIDs)message->m_header.m_messageID;
			switch (id)
			{
			case Test:
			{
				// show test value
				std::ostringstream ss;
				ss << "Received Test message containing value " << (uint32_t)((MessageTest*)message)->m_testValue;
				Log::Info(ss.str());
			}
			break;
			case DiscoveryRequest:
			{
				if (IsServer())
				{
					MessageDiscoveryRequest* request = (MessageDiscoveryRequest*)message;
					if (request->m_gameID == s_gameIdentifier)
					{
						Log::Info("Server received a DiscoveryRequest");

						// answer the request with server info
						std::unique_ptr<MessageDiscoveryAnswer> answer = MessageDiscoveryAnswer::Create();
						answer->m_gameID = s_gameIdentifier;
						answer->m_totalSlots = m_maxPeers;
						answer->m_freeSlots = m_maxPeers - (uint8_t)m_peers.size();
						sendMessage(peer->Address(), std::move(answer));
					}
					else
					{
						Log::Info("Server received a WRONG DiscoveryRequest");
					}
				}
				else
				{
					Log::Warn("Client received a DiscoveryRequest message");
				}
			}
			break;
			case DiscoveryAnswer:
			{
				if (IsServer())
				{
					Log::Warn("Server received a DiscoveryAnswer message");
				}
				else
				{
					Log::Info("Client received a DiscoveryAnswer");
					MessageDiscoveryAnswer* answer = (MessageDiscoveryAnswer*)message;
					if (answer->m_gameID == s_gameIdentifier)
					{
						// TODO: onDiscoveryAnswer callback
						if (answer->m_freeSlots > 0)
						{
							// FIXME: this is for testing
							ConnectTo(peer->Address());
						}
						else
						{
							Log::Info("Server found, but its full");
						}
					}
				}
			}
			break;
			case ConnectionRequest:
			{
				if (IsServer())
				{
					// only accept request from unknown peers
					if (peer->m_assignedID == 0xFF)
					{
						MessageConnectionRequest* request = (MessageConnectionRequest*)message;
						// check its a client with our same game
						if (request->m_gameID == s_gameIdentifier)
						{
							Log::Info("Server received a ConnectionRequest");
							// add a new peer entry if we have room for it
							uint8_t assignedID = AddPeer(peer->Address());
							if (assignedID != 0xFF)
							{
								// update the pointer
								peer = m_peers[assignedID];
								peer->SetSate(NetPeerState::Connecting);

								// send an answer with the assigned ID
								std::unique_ptr<MessageConnectionAnswer> answer = MessageConnectionAnswer::Create();
								answer->m_assignedID = assignedID;
								answer->m_challenge = s_challengeSeed;
								SendTo(assignedID, std::move(answer));
							}
							else
							{
								// send a quick answer telling we are full
								std::unique_ptr<MessageConnectionAnswer> answer = MessageConnectionAnswer::Create();
								answer->m_assignedID = 0xFF;
								sendMessage(peer->Address(), std::move(answer));
							}
						}
					}
				}
				else
				{
					Log::Warn("Client received a ConnectionRequest message");
				}
			}
			break;
			case ConnectionAnswer:
			{
				if (IsServer())
				{
					// first check the peer is connecting
					if (peer->State() == NetPeerState::Connecting)
					{
						MessageConnectionAnswer* answer = (MessageConnectionAnswer*)message;
						// check that the assigned ID and the challenge response are correct
						if ((answer->m_assignedID == peer->m_assignedID) &&
							(answer->m_challenge == generateChallengeResult(s_challengeSeed)))
						{
							Log::Info("Server received a valid ConnectionAnswer");
							// send the final message
							std::unique_ptr<MessageConnectionSuccess> answer = MessageConnectionSuccess::Create();
							answer->m_gameID = s_gameIdentifier;
							SendTo(peer, std::move(answer));
							// finally mark it as connected
							peer->SetSate(NetPeerState::Connected);
							OnConnection(peer->m_assignedID);
						}
						else
						{
							// the parameters are wrong, disconnect it
							DisconnectPeer(peer->m_assignedID, 1);
						}
					}
					else
					{
						Log::Warn("Server received a ConnectionAnswer from a non-connecting client");
					}
				}
				else
				{
					if (m_state == NetPeerState::Connecting)
					{
						MessageConnectionAnswer* serverAnswer = (MessageConnectionAnswer*)message;
						if (serverAnswer->m_assignedID != 0xFF)
						{
							Log::Info("Cliente received an assigned ConnectionAnswer");

							// save our id
							m_assignedID = serverAnswer->m_assignedID;

							// send another answer with challenge response
							std::unique_ptr<MessageConnectionAnswer> answer = MessageConnectionAnswer::Create();
							answer->m_assignedID = serverAnswer->m_assignedID;
							answer->m_challenge = generateChallengeResult(serverAnswer->m_challenge);
							SendTo(s_serverPeerID, std::move(answer));
						}
						else
						{
							// server is full
							Log::Info("Client received a \"server is full\" ConnectionAnswer");
							DisconnectAll();
							// TODO: maybe a callback on fail?
						}
					}
					else
					{
						Log::Warn("Client received a ConnectionAnswer without being connecting");
					}
				}
			}
			break;
			case ConnectionSuccess:
			{
				if (IsServer())
				{
					Log::Warn("Server received a ConnectionSuccess message");
				}
				else
				{
					Log::Warn("Client received a ConnectionSuccess message. We are connected!");
					// mark us as connected
					m_state = NetPeerState::Connected;
					OnConnection(m_assignedID);
				}
			}
			break;
			case KeepAlive:
			{
				// answer or update the ping and RTT based on how much time the answer delayed
				MessageKeepAlive* keepAlive = (MessageKeepAlive*)message;

				// if I sent it
				bool serverSent = (IsServer() && (keepAlive->m_serverSent != 0));
				bool clientSent = (!IsServer() && (keepAlive->m_serverSent == 0));
#if QUICKNET_VERBOSE
				std::ostringstream ss;
				ss << "KeepAlive received (remote: " << !(serverSent || clientSent) << ")";
				Log::Info(ss.str());
#endif
				if (serverSent || clientSent)
				{
					uint64_t milliseconds = Utils::GetElapsedMilliseconds() - keepAlive->m_timeStamp;
					peer->UpdateRTT((uint32_t)milliseconds);
				}
				else
				{
					// the other sent it, answer
					std::unique_ptr<MessageKeepAlive> answer = MessageKeepAlive::Create();
					answer->m_serverSent = IsServer() ? 0x00 : 0x01; // this is reversed here
					answer->m_timeStamp = keepAlive->m_timeStamp; // return the same timestamp (important)
					SendTo(peer, std::move(answer));
				}

			}
			break;
			case DisconnectionRequest:
			{
				if (IsServer())
				{
					if (peer->State() != NetPeerState::Disconnected)
					{
						// client quits
						OnDisconnection(peer->m_assignedID);
						DisconnectPeer(peer->m_assignedID, 0);
					}
					else
					{
						Log::Warn("Server received a DisconnectionRequest message from an unknown peer");
					}
				}
				else
				{
					// server disconnects client (me)
					OnDisconnection(m_assignedID);
					DisconnectAll();
				}
			}
			break;
			default:
			{
				std::ostringstream ss;
				ss << "Received an unknown message (ID: " << id << ") with system flag";
				Log::Warn(ss.str());
			}
			break;
			}
		}
		else
		{
			OnGameMessage(message);
		}
	}

	void Peer::processAcks(RemotePeer* peer, PacketHeader& header)
	{
		if (peer == nullptr) { return; }
#if QUICKNET_VERBOSE
		std::ostringstream ss;
		ss << "ACKS: sequence is " << header.m_ackseq << ". bits: " << header.m_ackbits;
		Log::Info(ss.str());
#endif
		peer->ProcessAckBits(header.m_ackseq, header.m_ackbits);
	}

	void Peer::send()
	{
		for (std::pair<const uint8_t, RemotePeer*>& peer : m_peers)
		{
			// check if its too early to send another packet
			if (peer.second->MillisecondsSinceLastSend() < m_sendTime)
			{
				continue;
			}

#if QUICKNET_VERBOSE
			//std::ostringstream ss;
			//ss << "elapsed " << peer.second->TicksSinceLastSend() << " of " << m_sendTime;
			//Log::Info(ss.str());
#endif

		// if we have something to send for this peer
			if (peer.second->HaveMessagesPending())
			{
				Packet packet;

				// first put the oldest ack-pending reliable
				packet.AddMessage(peer.second->DequeueReliableMessage());

				// if we have room for more messages
				while (packet.Size() < s_maximumPacketSize)
				{
					// try to add a pending message
					bool added = packet.AddMessage(peer.second->DequeueMessage());
					if (!added) { break; }
				}

				// if theres no messages, go to next peer
				if (packet.MessageCount() == 0) { continue; }

				// generate the headers for both packet and messages
				packet.GeneratePacketHeader(peer.second);
				packet.GenerateMessageHeaders(peer.second);

#if QUICKNET_VERBOSE
				std::ostringstream ss;
				ss << "Sending packet with " << packet.MessageCount() << " messages inside";
				Log::Info(ss.str());
#endif

				// serialize everything to the send buffer
				if (packet.ToBuffer(m_sendBuffer, s_bufferSize))
				{
					// dont send the message if fake packet loss quicks in
					if ((m_fakePacketLoss > 0.0f) && (m_rng.GetFloat() <= m_fakePacketLoss))
					{
						Log::Info("send: Fake Packet Loss kicked in!");
						peer.second->UpdateLastSend();
					}
					else
					{
						uint32_t sent = 0;
						bool success = m_socket.Send(peer.second->Address(), m_sendBuffer, packet.Size(), &sent);

						if (success)
						{
							peer.second->UpdateLastSend();
						}
						else
						{
							Log::Warn("Socket::Send failed!");
						}
					}
				}
				else
				{
					Log::Error("send: Packet::ToBuffer failed");
				}

				// return the reliable back to the peer
				packet.BackupReliables(peer.second);
			}
		}
	}

	bool Peer::sendMessage(const Address& address, std::unique_ptr<Message> message)
	{
#if QUICKNET_VERBOSE
		std::ostringstream ss;
		message->m_header = message->GenerateHeader();
		ss << "sendMessage: Sending direct message " << message->Name() << " to " << address.ToIPv4String();
		Log::Info(ss.str());
#endif

		Packet packet;
		packet.AddMessage(std::move(message));
		packet.GeneratePacketHeader(nullptr);
		packet.GenerateMessageHeaders(nullptr);

		if (packet.ToBuffer(m_sendBuffer, s_bufferSize))
		{
			// dont send the message if fake packet loss quicks in
			if ((m_fakePacketLoss > 0.0f) && (m_rng.GetFloat() <= m_fakePacketLoss))
			{
				Log::Info("sendMessage: Fake Packet Loss kicked in!");
				// but mark it as sent
				return true;
			}
			uint32_t sent = 0;
			bool success = m_socket.Send(address, m_sendBuffer, packet.Size(), &sent);
			return success;
		}
		else
		{
			Log::Error("sendMessage: Packet::ToBuffer failed");
		}
		return false;
	}

	RemotePeer* Peer::addressToPeer(Address& address)
	{
		auto it = m_addressIDs.find(address);

		if (it != m_addressIDs.end())
		{
			const uint8_t peerID = it->second;
			auto it = m_peers.find(peerID);
			if (it != m_peers.end())
			{
				return m_peers[peerID];
			}
		}
		return nullptr;
	}

	uint32_t Peer::generateChallengeResult(uint32_t challenge)
	{
		// TODO: this should be way more serious
		return (challenge ^ s_gameIdentifier);
	}
}