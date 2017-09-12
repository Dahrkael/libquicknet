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
// Packet is one or more Messages sent together with one PacketHeader
// The header contains a checksum for the whole packet and the acks for reliable messages
//

#pragma once
#include <stdint.h>
#include <deque>
#include <memory>

namespace quicknet
{
	class RemotePeer;
	class Stream;
	class Message;

	class PacketHeader
	{
	public:
		PacketHeader();
		~PacketHeader();

		bool FromBuffer(uint8_t* data, uint32_t length);
		bool ToBuffer(uint8_t* data, uint32_t length);

		bool FromStream(Stream& stream);
		bool ToStream(Stream& stream);

		bool IsChecksumValid(uint8_t* data, uint32_t length);
		void ComputeChecksum(uint8_t* data, uint32_t length);

		// total header size
		static uint32_t Size() { return (sizeof(uint16_t) * 2) + sizeof(uint32_t); }

		uint16_t m_checksum;
		uint16_t m_ackseq;
		uint32_t m_ackbits;
	};

	////////////////////////////////////////////////////////////////////////////////////////

	class Packet
	{
	public:
		Packet();
		~Packet();

		void GeneratePacketHeader(RemotePeer* peer);
		void GenerateMessageHeaders(RemotePeer* peer);
		bool AddMessage(std::unique_ptr<Message> message);
		void BackupReliables(RemotePeer* peer);

		bool ToBuffer(uint8_t* data, uint32_t length);
		bool ToStream(Stream& stream);

		uint32_t MessageCount() { return m_messages.size(); }
		uint32_t Size();

	private:
		PacketHeader m_header;
		std::deque<std::unique_ptr<Message>> m_messages;
	};
}