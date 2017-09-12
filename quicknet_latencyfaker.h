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

//
// implemented this separated to avoid code bloat in Peer
// FIXME: acknowledgments still get through
// FIXME: fake latency doesn't take real latency into account yet
//
#pragma once
#include <deque>
#include <stdint.h>
#include <memory>
#include "quicknet_address.h"
#include "quicknet_message.h"

namespace quicknet
{
	class RemotePeer;

	// FIXME: m_message should be a unique_ptr
	class NetLatencyFakerEntry
	{
	public:
		Message*	m_message;   // the actual message
		Address		m_address;   // where did it come from
		uint64_t	m_timeStamp; // when did it arrive
	};

	class NetLatencyFaker
	{
	public:
		NetLatencyFaker();
		~NetLatencyFaker();

		void SetLatency(uint32_t milliseconds) { m_latency = milliseconds; }
		uint32_t CurrentLatency() const { return m_latency; }

		// get a message which timestamp expired within the fake latency
		void GetMessageReady(std::unique_ptr<Message>& message, Address& address);
		// add a new message to the queue waiting to "arrive"
		void AddPendingMessage(std::unique_ptr<Message> message, RemotePeer* peer);

	private:
		bool shouldArrive(const NetLatencyFakerEntry& entry);

		// fake latency set
		uint32_t m_latency;

		// messages pending to "arrive"
		std::deque<NetLatencyFakerEntry> m_entries;
	};

}