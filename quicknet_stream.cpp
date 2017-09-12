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

#include "quicknet_stream.h"

namespace quicknet
{
	// to serialize floats
	union floatInt
	{
		float f;
		uint32_t i;
	};

	// TODO: this class needs to take endianess into account

	Stream::Stream(uint8_t* data, uint32_t length, NetStreamMode automode)
		: m_buffer(data)
		, m_length(length)
		, m_index(0)
		, m_mode(automode)
	{
		// TODO: assert data and length are valid

	}

	Stream::~Stream()
	{
	}

	// this would be a good place to use two templates instead of a lot of functions
	// then have a couple specialized like floats
	/*
	template <typename T>
	bool Stream::Write(T value)
	{
		if (fits(sizeof(T)))
		{
			*((T*)(m_buffer + m_index)) = value;
			m_index += sizeof(T);
			return true;
		}
		return false;
	}

	template <typename T>
	bool Stream::Read(T& value)
	{
		if (fits(sizeof(T)))
		{
			value = *((T*)(m_buffer + m_index));
			m_index += sizeof(T);
		}
		return value;
	}
	*/

	bool Stream::WriteByte(uint8_t value)
	{
		if (fits(sizeof(uint8_t)))
		{
			*(m_buffer + m_index) = value;
			m_index += sizeof(uint8_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteShort(int16_t value)
	{
		if (fits(sizeof(int16_t)))
		{
			*((int16_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(int16_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteUShort(uint16_t value)
	{
		if (fits(sizeof(uint16_t)))
		{
			*((uint16_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(uint16_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteInt(int32_t value)
	{
		if (fits(sizeof(int32_t)))
		{
			*((int32_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(int32_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteUInt(uint32_t value)
	{
		if (fits(sizeof(uint32_t)))
		{
			*((uint32_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(uint32_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteLong(int64_t value)
	{
		if (fits(sizeof(int64_t)))
		{
			*((int64_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(int64_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteULong(uint64_t value)
	{
		if (fits(sizeof(uint64_t)))
		{
			*((uint64_t*)(m_buffer + m_index)) = value;
			m_index += sizeof(uint64_t);
			return true;
		}
		return false;
	}

	bool Stream::WriteFloat(float value)
	{
		floatInt f;
		f.f = value;
		return WriteUInt(f.i);
	}

	bool Stream::WriteQFloat(float& value, float min, float max, float step)
	{
		QFloat qf(value, min, max, step);
		uint16_t qvalue;
		if (qf.ToUShort(qvalue))
		{
			return WriteUShort(qvalue);
		}
		return false;
	}


	bool Stream::ReadByte(uint8_t& value)
	{
		if (fits(sizeof(uint8_t)))
		{
			value = *(m_buffer + m_index);
			m_index += sizeof(uint8_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadShort(int16_t& value)
	{
		if (fits(sizeof(int16_t)))
		{
			value = *((int16_t*)(m_buffer + m_index));
			m_index += sizeof(int16_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadUShort(uint16_t& value)
	{
		if (fits(sizeof(uint16_t)))
		{
			value = *((uint16_t*)(m_buffer + m_index));
			m_index += sizeof(uint16_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadInt(int32_t& value)
	{
		if (fits(sizeof(int32_t)))
		{
			value = *((int32_t*)(m_buffer + m_index));
			m_index += sizeof(int32_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadUInt(uint32_t& value)
	{
		if (fits(sizeof(uint32_t)))
		{
			value = *((uint32_t*)(m_buffer + m_index));
			m_index += sizeof(uint32_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadLong(int64_t& value)
	{
		if (fits(sizeof(int64_t)))
		{
			value = *((int64_t*)(m_buffer + m_index));
			m_index += sizeof(int64_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadULong(uint64_t& value)
	{
		if (fits(sizeof(uint64_t)))
		{
			value = *((uint64_t*)(m_buffer + m_index));
			m_index += sizeof(uint64_t);
			return true;
		}
		return false;
	}

	bool Stream::ReadFloat(float& value)
	{
		floatInt f;
		if (ReadUInt(f.i))
		{
			value = f.f;
			return true;
		}
		return false;
	}

	bool Stream::ReadQFloat(float& value, float min, float max, float step)
	{
		uint16_t qvalue;
		if (ReadUShort(qvalue))
		{
			QFloat qf(qvalue, min, max, step);
			value = qf.m_value;
			return true;
		}
		return false;
	}


	bool Stream::DeSerializeByte(uint8_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadByte(value);  break;
		case NetStreamMode::Write: return WriteByte(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeShort(int16_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadShort(value);  break;
		case NetStreamMode::Write: return WriteShort(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeUShort(uint16_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadUShort(value);  break;
		case NetStreamMode::Write: return WriteUShort(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeInt(int32_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadInt(value);  break;
		case NetStreamMode::Write: return WriteInt(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeUInt(uint32_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadUInt(value);  break;
		case NetStreamMode::Write: return WriteUInt(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeLong(int64_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadLong(value);  break;
		case NetStreamMode::Write: return WriteLong(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeULong(uint64_t& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadULong(value);  break;
		case NetStreamMode::Write: return WriteULong(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeFloat(float& value)
	{
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadFloat(value);  break;
		case NetStreamMode::Write: return WriteFloat(value); break;
		}
		return false;
	}

	bool Stream::DeSerializeQFloat(float& value, float min, float max, float step)
	{
		// TODO: this only support uint16_t for now (only one used currently)
		switch (m_mode)
		{
		case NetStreamMode::Read:  return ReadQFloat(value, min, max, step);  break;
		case NetStreamMode::Write: return WriteQFloat(value, min, max, step); break;
		}
		return false;
	}

	bool Stream::fits(uint32_t size)
	{
		return !((m_index + size) > m_length);
	}

	QFloat::QFloat(float value, float min, float max, float step)
		: m_value(value)
		, m_min(min)
		, m_max(max)
		, m_step(step)
	{
	}

	QFloat::QFloat(uint32_t value, float min, float max, float step)
		: m_min(min)
		, m_max(max)
		, m_step(step)
	{
		float zor = (float)((double)value / (double)UINT32_MAX);
		m_value = denormalize(zor, m_min, m_max);
	}

	QFloat::QFloat(uint16_t value, float min, float max, float step)
		: m_min(min)
		, m_max(max)
		, m_step(step)
	{
		float zor = (float)value / (float)UINT16_MAX;
		m_value = denormalize(zor, m_min, m_max);
	}

	QFloat::QFloat(uint8_t value, float min, float max, float step)
		: m_min(min)
		, m_max(max)
		, m_step(step)
	{
		float zor = (float)value / (float)UINT8_MAX;
		m_value = denormalize(zor, m_min, m_max);
	}

	bool QFloat::ToUint(uint32_t& value)
	{
		// how many different values do we need
		uint32_t intervals = (uint32_t)((m_max - m_min) / m_step);
		if (intervals < UINT32_MAX)
		{
			// normalize
			float zor = normalize(m_value, m_min, m_max);
			uint32_t result = (uint32_t)(zor * UINT32_MAX);
			value = result > (UINT32_MAX - 1) ? (UINT32_MAX - 1) : result;
			return true;
		}
		return false;
	}

	bool QFloat::ToUShort(uint16_t& value)
	{
		// how many different values do we need
		uint32_t intervals = (uint32_t)((m_max - m_min) / m_step);
		if (intervals < UINT16_MAX)
		{
			// normalize
			float zor = normalize(m_value, m_min, m_max);
			uint16_t result = (uint16_t)(zor * UINT16_MAX);
			value = result > (UINT16_MAX - 1) ? (UINT16_MAX - 1) : result;
			return true;
		}
		return false;
	}

	bool QFloat::ToByte(uint8_t& value)
	{
		// how many different values do we need
		uint32_t intervals = (uint32_t)((m_max - m_min) / m_step);
		if (intervals < UINT8_MAX)
		{
			// normalize
			float zor = normalize(m_value, m_min, m_max);
			uint8_t result = (uint8_t)(zor * UINT8_MAX);
			value = result > (UINT8_MAX - 1) ? (UINT8_MAX - 1) : result;
			return true;
		}
		return false;
	}

	float QFloat::normalize(float value, float min, float max)
	{
		return (value - min) / (max - min);
	}

	float QFloat::denormalize(float value, float min, float max)
	{
		return min + (value * (max - min));
	}
}