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
#include <string>
#include <memory>
#include <vector>
#include "quicknet_message.h"

namespace quicknet
{
	class Stream;

	// all the packet IDs are defined here
	enum MessageIDs
	{
		None = 0,
		// system messages
		Test,
		DiscoveryRequest,
		DiscoveryAnswer,
		ConnectionRequest,
		ConnectionAnswer,
		ConnectionSuccess,
		KeepAlive,
		DisconnectionRequest,
		// game messages
		PlayerJoined,
		PlayerLeft,
		COUNT
	};

	// To declare a new Message follow the macro instructions below,
	// then define DeSerialize() in the .cpp

	// I don't really like macros, but in this case I think is cleaner
	// It works as:
	// DEFINE_QUICKNETMESSAGE_START(packet name, total fields size in bytes, flags);
	// uint*_t field1;
	// uint*_t field2;
	// ...
	// DEFINE_QUICKNETMESSAGE_END;

	///
#define DEFINE_QUICKNETMESSAGE_START(name, size, flags) \
		class Message ## name : public Message \
		{ \
		public: \
			static std::unique_ptr<Message ## name>		Create()								{ return std::unique_ptr<Message ## name>(new Message ## name()); } \
			virtual MessageHeader						GenerateHeader() override				{ return MessageHeader(Size(), 0x00, flags, MessageIDs:: ## name); } \
			virtual bool								FromStream(Stream& stream) override		{ return DeSerialize(stream); } \
			virtual bool								ToStream(Stream& stream) override		{ return DeSerialize(stream); } \
			bool										DeSerialize(Stream& stream); \
			virtual void								CopyTo(Message* other) override			{ Message ## name* copy = (Message ## name*)other; *copy = *this; } \
			virtual uint16_t							Size() const override					{ return size; } \
			virtual std::string							Name() const override					{ return std::string("" #name); }

#define DEFINE_QUICKNETMESSAGE_END }
///

//////////////////////////////////////////////////////////////////////////////////////////////////
// SYSTEM MESSAGES
//////////////////////////////////////////////////////////////////////////////////////////////////

	DEFINE_QUICKNETMESSAGE_START(Test, 1, s_flagSystem);
	uint8_t m_testValue;
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(DiscoveryRequest, 4, (s_flagSystem | s_flagUnsequenced));
	uint32_t m_gameID; // this will identify our game from others on the multicast (reduntant in any case)
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(DiscoveryAnswer, 6, (s_flagSystem | s_flagUnsequenced));
	uint32_t m_gameID;
	uint8_t  m_freeSlots;  // these two could be merged
	uint8_t  m_totalSlots; // as we only support 4 players
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(ConnectionRequest, 4, (s_flagSystem | s_flagReliable));
	uint32_t m_gameID; // the initial request will also contain a specific ID
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(ConnectionAnswer, 5, (s_flagSystem | s_flagReliable));
	uint8_t  m_assignedID; // this will be our player ID (0xFF means full)
	uint32_t m_challenge;  // this would be used to verify the client
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(ConnectionSuccess, 4, (s_flagSystem | s_flagReliable));
	uint32_t m_gameID;
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(KeepAlive, 9, s_flagSystem);
	uint8_t m_serverSent; // we need to know if its ours or not
	uint64_t m_timeStamp;
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(DisconnectionRequest, 4, s_flagSystem);
	uint32_t m_gameID; // specific ID to keep structure with Connection request
	DEFINE_QUICKNETMESSAGE_END;

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// GAME MESSAGES
	//////////////////////////////////////////////////////////////////////////////////////////////////

	DEFINE_QUICKNETMESSAGE_START(PlayerJoined, 1, s_flagReliable);
	uint8_t m_playerID;
	DEFINE_QUICKNETMESSAGE_END;

	DEFINE_QUICKNETMESSAGE_START(PlayerLeft, 1, s_flagReliable);
	uint8_t m_playerID;
	DEFINE_QUICKNETMESSAGE_END;
}