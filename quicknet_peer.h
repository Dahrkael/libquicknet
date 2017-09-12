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

#pragma once
#include <unordered_map> // O(1) find() vs O(logN) of normal map

#define QUICKNET_VERBOSE 0

#include "quicknet_address.h"
#include "quicknet_udpsocket.h"
#include "quicknet_latencyfaker.h"
#include "quicknet_fastrand.h"

namespace quicknet
{
	class RemotePeer;
	class Message;
	class PacketHeader;

	enum NetPeerState
	{
		Disconnected,
		Searching,
		Connecting,
		Connected,
		ServerMode
	};

	class Peer
	{
	public:
		// max peers only apply to server, clients only allow 1 connection
		Peer(bool serverMode, uint8_t maxPeers);
		~Peer();

		// find servers through broadcast on LAN
		bool FindServers();
		// connect to a local or Internet server
		bool ConnectTo(const Address& address);
		// send disconnection message & remove all remote peers
		void DisconnectAll();
		// receive and process packets & update peers state
		void UpdateNetwork();
		// send message to specific remote peer
		bool SendTo(uint8_t peerID, std::unique_ptr<Message> message);
		// send message to specific remote peer
		bool SendTo(RemotePeer* peer, std::unique_ptr<Message> message);
		// send message to all the remote peers
		bool SendToAll(std::unique_ptr<Message> message);

		// we need to select the mode on runtime
		void SetServerMode(bool enable);

		// set a fake packet loss from 0.0f to 1.0f
		void  SetFakePacketLoss(float percentage);
		float CurrentFakePacketLoss() const { return m_fakePacketLoss; }

		// set fake latency in milliseconds
		void  SetFakeLatency(uint32_t milliseconds);
		uint32_t CurrentFakeLatency() const { return m_fakeLatency.CurrentLatency(); }

		// current state and mode getters
		const NetPeerState NetworkState() const { return m_state; }
		// round trip time from client to server or average from server to clients
		const uint32_t RTT();
		const bool IsServer() const { return m_state == NetPeerState::ServerMode; }

		const uint8_t AssignedID() const { return m_assignedID; }
	protected:
		virtual void OnConnection(uint8_t playerID) = 0;
		virtual void OnDisconnection(uint8_t peerID) = 0;
		// this is where the actual game events will be processed
		virtual void OnGameMessage(const Message* const message) = 0;

	private:
		// add a new peer
		uint8_t AddPeer(const Address& address);
		// send disconnection message & remove peer
		bool DisconnectPeer(uint8_t peerID, uint8_t amount = 5);

	private:
		// find a free peer ID
		uint8_t findFreePeerID();
		// check if a peer is associated with that ID
		bool peerExists(uint8_t peerID);
		// do maintenance stuff on all peers
		void updatePeers();
		// receive packets for processing
		void receive();
		// parse the recv buffer
		void parseBuffer(uint32_t length, RemotePeer* peer);
		// process new packets
		void processMessage(const Message* const message, RemotePeer* peer);
		// update peers state based on new data
		void processAcks(RemotePeer* peer, PacketHeader& header);
		// do the actual send with the specified rate 
		// and merge the packets to save calls
		void send();
		// send one message directly to the specified address
		bool sendMessage(const Address& address, std::unique_ptr<Message> message);

		RemotePeer* addressToPeer(Address& address);
		uint32_t generateChallengeResult(uint32_t challenge);

		// local peer state
		NetPeerState m_state;
		UDPSocket m_socket;
		uint8_t m_assignedID;
		// peer list and lookup
		std::unordered_map<uint8_t, RemotePeer*> m_peers;
		std::unordered_map<Address, uint8_t, NetAddressHasher> m_addressIDs;
		// maximum amount of connected peers allowed
		uint8_t m_maxPeers;
		// to keep track of send rate
		uint64_t m_lastSend;
		// send&receive buffers
		uint8_t* m_recvBuffer;
		uint8_t* m_sendBuffer;

		// debugging
		FastRand m_rng;
		float m_fakePacketLoss;
		NetLatencyFaker m_fakeLatency;
	};
}