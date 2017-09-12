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
#include "quicknet_includes.h"

namespace quicknet
{
	class Address;

#ifdef _WIN32
	typedef SOCKET RawSocket; // uint64_t
#else
	typedef int32_t RawSocket;
#endif

	class UDPSocket
	{
	public:
		UDPSocket(RawSocket udpSocket);
		UDPSocket(bool isIPv6 = false, int32_t timeout = 0);
		~UDPSocket();

		// basic methods
		bool Close();
		bool Bind(const Address& address);
		bool Send(const Address& remote, uint8_t* data, uint32_t dataLength, uint32_t* bytesSent);
		bool Recv(uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead, Address& remote);

		// handy methods
		bool SetTimeout(int32_t timeout);
		bool AllowBroadcast(bool allow);
		bool IsValid();

	private:
		bool setBlockingMode(bool blocking);
		uint32_t getLastNetworkError();

		RawSocket m_udpSocket;
	};
}