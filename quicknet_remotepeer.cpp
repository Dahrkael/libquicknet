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

#include <sstream>
#include "quicknet_remotepeer.h"
#include "quicknet_messageslookup.h"

namespace quicknet
{
	RemotePeer::RemotePeer(quicknet::Address address)
		: m_address(address)
		, m_assignedID(0xFF)
		, m_state(NetPeerState::Disconnected)
		, m_ping(0)
		, m_rtt(0)
		, m_sequenceIn(0)
		, m_sequenceOut(1)
		, m_sequenceRound(false)
		, m_seqtrackReceived()
		, m_pendingMessages()
		, m_reliableMessages()
		, m_lastAckTime(Utils::GetElapsedMilliseconds())
		, m_lastMessageTime(Utils::GetElapsedMilliseconds())
		, m_lastSend(0)
	{
	}

	RemotePeer::~RemotePeer()
	{
	}

	void RemotePeer::EnqueueMessage(std::unique_ptr<Message> message)
	{
		m_pendingMessages.push_back(std::move(message));
	}

	void RemotePeer::RequeueMessage(std::unique_ptr<Message> message)
	{
		if (!message->m_header.IsReliable()) { return; }

		// set the last send timestamp
		m_seqtrackSent[message->m_header.m_sequence] = Utils::GetElapsedMilliseconds();

		// put it on the back so we rotate the messages
		// since we are sending one per message @ 20/s
		m_reliableMessages.push_back(std::move(message));
	}

	std::unique_ptr<Message> RemotePeer::DequeueMessage()
	{
		if (m_pendingMessages.empty()) { return nullptr; }

		std::unique_ptr<Message> message = std::move(m_pendingMessages.front());
		m_pendingMessages.pop_front();

		return message;
	}

	std::unique_ptr<Message> RemotePeer::DequeueReliableMessage()
	{
		if (m_reliableMessages.empty()) { return nullptr; }

		std::unique_ptr<Message> message = std::move(m_reliableMessages.front());
		m_reliableMessages.pop_front();

		return message;
	}

	void RemotePeer::UpdateRTT(uint32_t milliseconds)
	{
		m_ping = milliseconds / 2;
		m_rtt = (m_rtt == 0) ? milliseconds : (((m_rtt * 90) + (milliseconds * 10)) / 100);

#if QUICKNET_VERBOSE
		std::ostringstream ss;
		ss << "Peer " << (uint32_t)m_assignedID << " current RTT: " << RTT() << " Ping: " << Ping();
		Log::Info(ss.str());
#endif
	}

	uint32_t RemotePeer::GetAckBits()
	{
		uint32_t bits = 0x00;
		uint16_t first = CurrentSequenceIn() - 1;
		for (uint16_t i = 0; i < 32; i++)
		{
			uint16_t current = first - i;
			uint32_t round = m_sequenceRound;

			auto it = m_seqtrackReceived.find(current);
			if (it != m_seqtrackReceived.end())
			{
				if (current > first) { --round; }
				if (it->second.m_round == round)
				{
					bitSet(&bits, i);
				}
			}
		}
		return bits;
	}

	void RemotePeer::ProcessAckBits(uint16_t sequence, uint32_t ackbits)
	{
		// check the ack-pending messages and remove those that match the ack sequences

		// first the base sequence
		for (auto it = m_reliableMessages.begin(); it != m_reliableMessages.end(); it++)
		{
			if ((*it)->m_header.m_sequence == sequence)
			{
#if QUICKNET_VERBOSE
				Log::Info("ACKS: acked reliable sequence found. deleting");
#endif
				// first update the RTT
				{
					auto time = m_seqtrackSent.find(sequence);
					if (time != m_seqtrackSent.end())
					{
						uint32_t milliseconds = (uint32_t)(Utils::GetElapsedMilliseconds() - time->second);
						UpdateRTT(milliseconds);
					}
					m_seqtrackSent.erase(sequence);
				}
				m_reliableMessages.erase(it);
				// once deleted we can stop searching
				break;
			}
		}

		uint16_t first = sequence - 1;
		for (uint16_t i = 0; i < 32; i++)
		{
			// if the bit is set, then the message is acknowledged
			if (bitCheck(ackbits, i))
			{
				uint16_t current = first - i;
				// search the sequence and remove the message
				for (auto it = m_reliableMessages.begin(); it != m_reliableMessages.end(); it++)
				{
					if ((*it)->m_header.m_sequence == current)
					{
#if QUICKNET_VERBOSE
						Log::Info("ACKS: acked reliable sequence found. deleting");
#endif
						// first update the RTT
						{
							auto time = m_seqtrackSent.find(current);
							if (time != m_seqtrackSent.end())
							{
								uint32_t milliseconds = (uint32_t)(Utils::GetElapsedMilliseconds() - time->second);
								UpdateRTT(milliseconds);
							}
							m_seqtrackSent.erase(current);
						}
						m_reliableMessages.erase(it);
						// once deleted we can stop searching
						break;
					}
				}
			}
		}

		UpdateLastAckTime();
	}

	void RemotePeer::SetSequenceIn(uint16_t value)
	{
		if (value < m_sequenceIn)
		{
			m_sequenceRound++;
		}
		m_sequenceIn = value;
	}

	bool RemotePeer::IsSequenceNewer(uint16_t incoming, uint16_t current)
	{
		if (incoming == current) { return false; }

		// TODO: if we have a jump bigger than 'half' it will break
		// but that would mean an unplayable packet loss
		const int32_t half = 32768; // 65536/2
		int32_t diff = abs((int32_t)incoming - (int32_t)current);
		uint16_t bigger = incoming > current ? incoming : current;

		if (diff < half)
		{
			return (bigger == incoming);
		}
		else
		{
			return (bigger == current);
		}
	}

	void RemotePeer::SaveReceivedSequence(uint16_t sequence, bool newer)
	{
		SequenceTrackingEntry entry;
		entry.m_round = m_sequenceRound;

		if (!newer)
		{
			if (sequence > CurrentSequenceIn())
			{
				entry.m_round = m_sequenceRound - 1;
			}
		}

#if QUICKNET_VERBOSE
		std::ostringstream ss;
		ss << "Saving recv sequence " << sequence << " on round " << entry.m_round;
		Log::Info(ss.str());
#endif
		m_seqtrackReceived[sequence] = entry;
	}

	bool RemotePeer::MessageDuplicated(uint16_t sequence)
	{
		bool result = false;
		// which round to check
		uint32_t round = m_sequenceRound;

		auto it = m_seqtrackReceived.find(sequence);
		if (it != m_seqtrackReceived.end())
		{
			// if its a message from before last overflow, search the round before
			if (sequence > CurrentSequenceIn())
			{
				--round;
			}
			result = (it->second.m_round == round);
		}

#if QUICKNET_VERBOSE
		if (result)
		{
			std::ostringstream ss;
			ss << "Duplicated sequence " << sequence << " on round " << round;
			Log::Info(ss.str());
		}
#endif
		return result;
	}
}