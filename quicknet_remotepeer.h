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
#include <stdint.h>
#include <memory>
#include <deque>
#include "quicknet_address.h"
#include "quicknet_peer.h"
#include "quicknet_message.h"
#include "quicknet_time.h"

namespace quicknet
{
	struct SequenceTrackingEntry
	{
		uint32_t m_round; // so we know if the sequence already overflowed
	};

	class RemotePeer
	{
	public:
		RemotePeer(Address address);
		~RemotePeer();

		// address getter
		const Address Address() const { return m_address; }

		// getter/setter for the state
		const NetPeerState State() const { return m_state; }
		void SetSate(NetPeerState state) { m_state = state; }

		// add message to send
		void EnqueueMessage(std::unique_ptr<Message> message);
		// add message to wait for ack
		void RequeueMessage(std::unique_ptr<Message> message);

		// get send-pending message
		std::unique_ptr<Message> DequeueMessage();
		// get ack-pending messages
		std::unique_ptr<Message> DequeueReliableMessage();

		// check if theres new messages to send
		bool HaveMessagesPending() { return !m_pendingMessages.empty(); }
		// check if theres non-ack'd reliables
		bool HaveReliableMessagesPending() { return !m_reliableMessages.empty(); }

		void UpdateRTT(uint32_t milliseconds);
		const uint32_t Ping() const { return m_ping; }
		const uint32_t RTT()  const { return m_rtt; }

		// sequence getters
		const uint16_t CurrentSequenceIn()  const { return m_sequenceIn; }
		const uint16_t CurrentSequenceOut() const { return m_sequenceOut; }

		// sequence setters
		void SetSequenceIn(uint16_t value);
		void SetSequenceOut(uint16_t value) { m_sequenceOut = value; }

		// comparison taking overflow into account
		bool IsSequenceNewer(uint16_t incoming, uint16_t current);

		// sequence tracking
		void SaveReceivedSequence(uint16_t sequence, bool newer);
		bool MessageDuplicated(uint16_t sequence);

		// get a bitfield to acknowledge the last 32 messages
		uint32_t GetAckBits();
		void ProcessAckBits(uint16_t sequence, uint32_t ackbits);

		uint64_t MillisecondsSinceLastMessage() { return Utils::GetElapsedMilliseconds() - m_lastMessageTime; }
		void  UpdateLastMessageTime() { m_lastMessageTime = Utils::GetElapsedMilliseconds(); }

		// check if we need to send KeepAlives
		uint64_t MillisecondsSinceLastAck() { return Utils::GetElapsedMilliseconds() - m_lastAckTime; }
		void  UpdateLastAckTime() { m_lastAckTime = Utils::GetElapsedMilliseconds(); }

		uint64_t MillisecondsSinceLastSend() const { return Utils::GetElapsedMilliseconds() - m_lastSend; }
		void UpdateLastSend() { m_lastSend = Utils::GetElapsedMilliseconds(); }

		// the ID in Peer m_peers
		uint8_t m_assignedID;
	private:
		// current connection state
		NetPeerState m_state;
		// the remote address
		quicknet::Address m_address;
		// raw and smoothed latency values
		uint32_t m_ping;
		uint32_t m_rtt;
		// current sequence id for both directions
		uint16_t m_sequenceIn;
		uint16_t m_sequenceOut;
		// this will change when the 'in' sequence overflows
		uint32_t m_sequenceRound;
		// hash map to keep track of message sequences
		std::unordered_map<uint16_t, SequenceTrackingEntry> m_seqtrackReceived;
		// hash map to keep track of reliable messages send times
		std::unordered_map<uint16_t, uint64_t> m_seqtrackSent;
		// queue of pending messages to send
		std::deque<std::unique_ptr<Message>>  m_pendingMessages;
		// queue of sent ack-pending reliable messages
		std::deque<std::unique_ptr<Message>> m_reliableMessages;
		// last ack time
		uint64_t m_lastAckTime;
		// last time we received a message
		uint64_t m_lastMessageTime;
		// last time we sent something
		uint64_t m_lastSend;
	};
}