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

#include "quicknet_message.h"
#include "quicknet_stream.h"
#include "quicknet_messageslookup.h"

namespace quicknet
{
	MessageHeader::MessageHeader()
		: m_size(0)
		, m_sequence(0)
		, m_flags(0)
		, m_messageID(0)
	{
	}

	MessageHeader::MessageHeader(uint16_t size, uint16_t sequence, uint8_t flags, uint8_t id)
		: m_size(size)
		, m_sequence(sequence)
		, m_flags(flags)
		, m_messageID(id)
	{
	}

	MessageHeader::~MessageHeader()
	{
	}

	bool MessageHeader::FromBuffer(uint8_t* data, uint32_t length)
	{
		Stream stream(data, length, NetStreamMode::Read);
		return FromStream(stream);
	}

	bool MessageHeader::ToBuffer(uint8_t* data, uint32_t length)
	{
		Stream stream(data, length, NetStreamMode::Write);
		return ToStream(stream);
	}

	bool MessageHeader::FromStream(Stream& stream)
	{
		bool success = true;
		success = success && stream.ReadUShort(m_size);
		success = success && stream.ReadUShort(m_sequence);
		success = success && stream.ReadByte(m_flags);
		success = success && stream.ReadByte(m_messageID);

		return success;
	}

	bool MessageHeader::ToStream(Stream& stream)
	{
		bool success = true;
		success = success && stream.WriteUShort(m_size);
		success = success && stream.WriteUShort(m_sequence);
		success = success && stream.WriteByte(m_flags);
		success = success && stream.WriteByte(m_messageID);

		return success;
	}

	bool MessageHeader::IsUnsequenced() const
	{
		return flagCheck(m_flags, s_flagUnsequenced);
	}

	bool MessageHeader::IsReliable() const
	{
		return flagCheck(m_flags, s_flagReliable);
	}

	bool MessageHeader::IsOrdered() const
	{
		return flagCheck(m_flags, s_flagOrdered);
	}

	bool MessageHeader::IsSystem() const
	{
		return flagCheck(m_flags, s_flagSystem);
	}

	////////////////////////////////////////////////////////////////////////////////////////

	bool Message::FromBuffer(uint8_t* data, uint32_t length)
	{
		Stream stream(data, length, NetStreamMode::Read);
		return FromStream(stream);
	}

	bool Message::ToBuffer(uint8_t* data, uint32_t length)
	{
		Stream stream(data, length, NetStreamMode::Write);
		return ToStream(stream);
	}
}