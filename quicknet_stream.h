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

// Stream is an abstraction for sequentially reading and writing buffers
// it simplifies the FromStream and ToStream interfaces for packets and messages, allowing to write only one method for both

namespace quicknet
{
	enum NetStreamMode
	{
		Read,
		Write
	};

	// quantized float
	class QFloat
	{
	public:
		QFloat(float value, float min, float max, float step);
		QFloat(uint32_t value, float min, float max, float step);
		QFloat(uint16_t value, float min, float max, float step);
		QFloat(uint8_t value, float min, float max, float step);

		bool ToUint(uint32_t& value);
		bool ToUShort(uint16_t& value);
		bool ToByte(uint8_t& value);

		float m_value;
		float m_min;
		float m_max;
		float m_step;

	private:
		float normalize(float value, float min, float max);
		float denormalize(float value, float min, float max);
	};

	class Stream
	{
	public:
		Stream(uint8_t* data, uint32_t length, NetStreamMode automode);
		~Stream();

		// helpers
		bool Full() { return !(m_index < m_length); }
		void Skip(uint32_t bytes) { m_index = (m_index + bytes > m_length) ? m_length : (m_index + bytes); }
		void Rewind(uint32_t bytes) { m_index = (m_index - bytes > m_index) ? 0 : (m_index - bytes); };

		// manual write
		bool WriteByte(uint8_t value);
		bool WriteShort(int16_t value);
		bool WriteUShort(uint16_t value);
		bool WriteInt(int32_t value);
		bool WriteUInt(uint32_t value);
		bool WriteLong(int64_t value);
		bool WriteULong(uint64_t value);
		bool WriteFloat(float value);
		bool WriteQFloat(float& value, float min, float max, float step);

		// manual read
		bool ReadByte(uint8_t& value);
		bool ReadShort(int16_t& value);
		bool ReadUShort(uint16_t& value);
		bool ReadInt(int32_t& value);
		bool ReadUInt(uint32_t& value);
		bool ReadLong(int64_t& value);
		bool ReadULong(uint64_t& value);
		bool ReadFloat(float& value);
		bool ReadQFloat(float& value, float min, float max, float step);

		// automatic methods
		bool DeSerializeByte(uint8_t& value);
		bool DeSerializeShort(int16_t& value);
		bool DeSerializeUShort(uint16_t& value);
		bool DeSerializeInt(int32_t& value);
		bool DeSerializeUInt(uint32_t& value);
		bool DeSerializeLong(int64_t& value);
		bool DeSerializeULong(uint64_t& value);
		bool DeSerializeFloat(float& value);

		bool DeSerializeQFloat(float& value, float min, float max, float step);

	private:
		bool fits(uint32_t size);

		uint8_t* m_buffer;
		uint32_t m_length;
		uint32_t m_index;
		NetStreamMode m_mode;
	};
}