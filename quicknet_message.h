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
// Message is one individual game or management mesage containing specific information
// Each Message has a MessageHeader with its sequence number and flags (reliable, etc)
//

#pragma once
#include <stdint.h>
#include <string>

namespace quicknet
{
	class Stream;

	// FLAGS
	static const uint32_t s_flagSystem		= (0x01);
	static const uint32_t s_flagReliable	= (0x01 << 1);
	static const uint32_t s_flagOrdered		= (0x01 << 2);
	static const uint32_t s_flagUnsequenced = (0x01 << 3);

	static void bitSet(uint32_t* bitfield, uint32_t bit)	{ *bitfield |= (1 << bit); }
	static void bitClear(uint32_t* bitfield, uint32_t bit)	{ *bitfield &= ~(1 << bit); }
	static void bitFlip(uint32_t* bitfield, uint32_t bit)	{ *bitfield ^= (1 << bit); }
	static bool bitCheck(uint32_t bitfield, uint32_t bit)	{ return (bitfield & (1 << bit)) != 0; }
	static bool flagCheck(uint32_t bitfield, uint32_t flag) { return (bitfield & flag) != 0; }
	//

	class MessageHeader
	{
	public:
		MessageHeader();
		MessageHeader(uint16_t size, uint16_t sequence, uint8_t flags, uint8_t id);
		~MessageHeader();

		bool FromBuffer(uint8_t* data, uint32_t length);
		bool ToBuffer(uint8_t* data, uint32_t length);

		bool FromStream(Stream& stream);
		bool ToStream(Stream& stream);

		// need to check the sequence?
		bool IsUnsequenced() const;
		// is it important?
		bool IsReliable() const;
		// should we care about its sequence?
		bool IsOrdered() const;
		// is it a management or a game message?
		bool IsSystem() const;

		// total header size
		static uint32_t Size() { return (sizeof(uint16_t) * 2) + (sizeof(uint8_t) * 2); }

		uint16_t m_size; // does not include header size
		uint16_t m_sequence;
		uint8_t  m_flags;
		uint8_t  m_messageID;
	};

	class Message
	{
	public:
		Message() : m_header() {}

		virtual MessageHeader GenerateHeader() = 0;

		bool FromBuffer(uint8_t* data, uint32_t length);
		bool ToBuffer(uint8_t* data, uint32_t length);

		virtual bool FromStream(Stream& stream) = 0;
		virtual bool ToStream(Stream& stream) = 0;

		virtual void CopyTo(Message* other) = 0;

		virtual uint16_t Size() const = 0;
		virtual std::string Name() const = 0;

		MessageHeader m_header;
	};

}