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

#include "quicknet_packet.h"
#include "quicknet_message.h"
#include "quicknet_stream.h"
#include "quicknet_remotepeer.h"

namespace quicknet
{
	uint16_t CRC16(const uint8_t* data, uint16_t length);

	PacketHeader::PacketHeader()
	{
	}

	PacketHeader::~PacketHeader()
	{
	}

	bool PacketHeader::FromBuffer(uint8_t* data, uint32_t length)
	{
		if (length < PacketHeader::Size()) { return false; }

		Stream stream(data, length, NetStreamMode::Read);
		return FromStream(stream);
	}

	bool PacketHeader::ToBuffer(uint8_t* data, uint32_t length)
	{
		if (length < PacketHeader::Size()) { return false; }

		Stream stream(data, length, NetStreamMode::Write);
		return ToStream(stream);
	}

	bool PacketHeader::FromStream(Stream& stream)
	{
		bool success = true;
		success = success && stream.ReadUShort(m_checksum);
		success = success && stream.ReadUShort(m_ackseq);
		success = success && stream.ReadUInt(m_ackbits);

		return success;
	}

	bool PacketHeader::ToStream(Stream& stream)
	{
		bool success = true;
		success = success && stream.WriteUShort(m_checksum);
		success = success && stream.WriteUShort(m_ackseq);
		success = success && stream.WriteUInt(m_ackbits);

		return success;
	}

	bool PacketHeader::IsChecksumValid(uint8_t* data, uint32_t length)
	{
		// make the calculation starting from the sequence
		uint16_t crc = CRC16(data + sizeof(uint16_t), length - sizeof(uint16_t));
		return (crc == m_checksum);
	}

	void PacketHeader::ComputeChecksum(uint8_t* data, uint32_t length)
	{
		// make the calculation starting from the sequence
		uint16_t crc = CRC16(data + sizeof(uint16_t), length - sizeof(uint16_t));
		m_checksum = crc;
	}

	////////////////////////////////////////////////////////////////////////////////////////

	Packet::Packet()
	{
	}

	Packet::~Packet()
	{
	}

	void Packet::GeneratePacketHeader(RemotePeer* peer)
	{
		// checksum is computed later
		m_header.m_checksum = 0x00;

		if (peer == nullptr)
		{
			m_header.m_ackseq = 0x00;
			m_header.m_ackbits = 0x00;
		}
		else
		{
			m_header.m_ackseq = peer->CurrentSequenceIn();
			m_header.m_ackbits = peer->GetAckBits();
		}
	}

	void Packet::GenerateMessageHeaders(RemotePeer* peer)
	{
		for (std::unique_ptr<Message>& message : m_messages)
		{
			// if the message has a sequence
			if (message->m_header.m_sequence != 0x00)
			{
				// skip it, because its an ack-pending reliable
				continue;
			}

			MessageHeader header = message->GenerateHeader();
			message->m_header = header;

			if (peer != nullptr)
			{
				// increase the sequence with every message
				message->m_header.m_sequence = peer->CurrentSequenceOut();
				peer->SetSequenceOut(peer->CurrentSequenceOut() + 1);
			}
		}
	}

	bool Packet::AddMessage(std::unique_ptr<Message> message)
	{
		if (!message) { return false; }

		m_messages.push_back(std::move(message));
		return true;
	}

	void Packet::BackupReliables(RemotePeer* peer)
	{
		// packet is destroyed right after, so we can move the messages freely
		for (std::unique_ptr<Message>& message : m_messages)
		{
			if (message->m_header.IsReliable())
			{
				peer->RequeueMessage(std::move(message));
			}
		}
	}

	bool Packet::ToBuffer(uint8_t* data, uint32_t length)
	{
		Stream stream(data, length, NetStreamMode::Write);
		bool success = ToStream(stream);
		// if everythings fine, get the checksum and write it too
		if (success)
		{
			m_header.ComputeChecksum(data, Size());
			stream.Rewind(length);
			stream.WriteUShort(m_header.m_checksum);
		}
		return success;
	}

	bool Packet::ToStream(Stream& stream)
	{
		bool success = true;
		// write everything in order
		success = success && m_header.ToStream(stream);
		for (std::unique_ptr<Message>& message : m_messages)
		{
			success = success && message->m_header.ToStream(stream);
			success = success && message->ToStream(stream);
		}

		return success;
	}

	uint32_t Packet::Size()
	{
		// TODO: maybe cache the size once computed?
		uint32_t size = PacketHeader::Size();
		for (const std::unique_ptr<Message>& message : m_messages)
		{
			size += MessageHeader::Size();
			size += message->Size();
		}
		return size;
	}


	// for the packet checksums
	uint16_t CRC16(const uint8_t* data, uint16_t length)
	{
		uint8_t x;
		uint16_t crc = 0xDEAD; // our special, hopefully unique, protocol key

		while (length--)
		{
			x = crc >> 8 ^ *(data++);
			x ^= x >> 4;
			crc = (crc << 8) ^ ((x << 12)) ^ ((x << 5)) ^ (x);
			crc &= 0xFFFF;
		}
		return crc;
	}
}