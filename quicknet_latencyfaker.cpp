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

#include "quicknet_latencyfaker.h"
#include "quicknet_remotepeer.h"
#include "quicknet_time.h"

namespace quicknet
{
	NetLatencyFaker::NetLatencyFaker()
		: m_entries()
		, m_latency(0)
	{
	}

	NetLatencyFaker::~NetLatencyFaker()
	{
		for (NetLatencyFakerEntry& entry : m_entries)
		{
			delete entry.m_message;
		}
		m_entries.clear();
	}

	void NetLatencyFaker::GetMessageReady(std::unique_ptr<Message>& message, Address& address)
	{
		if (m_entries.empty())
		{
			message = nullptr;
			return;
		}

		NetLatencyFakerEntry& entry = m_entries.front();
		if (shouldArrive(entry))
		{
			message = std::unique_ptr<Message>(entry.m_message);
			address = entry.m_address;
			m_entries.pop_front();
		}
		else
		{
			message = nullptr;
		}
	}

	void NetLatencyFaker::AddPendingMessage(std::unique_ptr<Message> message, RemotePeer* peer)
	{
		NetLatencyFakerEntry entry;
		entry.m_message = message.release();
		entry.m_address = peer->Address();
		entry.m_timeStamp = Utils::GetElapsedMilliseconds();

		m_entries.push_back(std::move(entry));
	}

	bool NetLatencyFaker::shouldArrive(const NetLatencyFakerEntry& entry)
	{
		// if fake latency is off, validate all messages
		if (m_latency == 0) { return true; }

		// how many ticks elapsed since arrival
		uint32_t diff = (uint32_t)(Utils::GetElapsedMilliseconds() - entry.m_timeStamp);

		return (diff >= m_latency);
	}
}