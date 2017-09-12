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

#include "quicknet_messagetypes.h"
#include "quicknet_messageslookup.h"
#include "quicknet_stream.h"

namespace quicknet
{
	bool MessageTest::DeSerialize(Stream& stream)
	{
		bool success = true;
		stream.DeSerializeByte(m_testValue);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageDiscoveryRequest::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeUInt(m_gameID);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageDiscoveryAnswer::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeUInt(m_gameID);
		success = success && stream.DeSerializeByte(m_freeSlots);
		success = success && stream.DeSerializeByte(m_totalSlots);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageConnectionRequest::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeUInt(m_gameID);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageConnectionAnswer::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeByte(m_assignedID);
		success = success && stream.DeSerializeUInt(m_challenge);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageConnectionSuccess::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeUInt(m_gameID);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageKeepAlive::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeByte(m_serverSent);
		success = success && stream.DeSerializeULong(m_timeStamp);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessageDisconnectionRequest::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeUInt(m_gameID);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////

	bool MessagePlayerJoined::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeByte(m_playerID);
		return success;
	}

	/////////////////////////////////////////////////////////////////////

	bool MessagePlayerLeft::DeSerialize(Stream& stream)
	{
		bool success = true;
		success = success && stream.DeSerializeByte(m_playerID);
		return success;
	}
}