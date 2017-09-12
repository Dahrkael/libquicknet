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
#include <sstream>
#include <memory>

#include "quicknet_message.h"
#include "quicknet_messagetypes.h"
#include "quicknet_log.h"

namespace quicknet
{
	// TODO: I dont like all these allocations, make a pool

	// TODO: maybe use a macro to avoid human errors declaring all these cases
#define CASE_GET_MESSAGE_FROM_ID(name) case name: \
{ \
	return Message ## name ## ::Create(); \
} \
break

	static std::unique_ptr<Message> GetMessageFromID(MessageIDs id)
	{
		switch (id)
		{
			// system messages
			CASE_GET_MESSAGE_FROM_ID(Test);
			CASE_GET_MESSAGE_FROM_ID(DiscoveryRequest);
			CASE_GET_MESSAGE_FROM_ID(DiscoveryAnswer);
			CASE_GET_MESSAGE_FROM_ID(ConnectionRequest);
			CASE_GET_MESSAGE_FROM_ID(ConnectionAnswer);
			CASE_GET_MESSAGE_FROM_ID(ConnectionSuccess);
			CASE_GET_MESSAGE_FROM_ID(KeepAlive);
			CASE_GET_MESSAGE_FROM_ID(DisconnectionRequest);
			// game messages
			CASE_GET_MESSAGE_FROM_ID(PlayerJoined);
			CASE_GET_MESSAGE_FROM_ID(PlayerLeft);
		case COUNT:
		default:
			std::ostringstream ss;
			ss << "GetMessageFromID: Requested ID " << id << " which doesn't have a statement";
			Log::Error(ss.str());
			break;
		}

		return nullptr;
	}
}