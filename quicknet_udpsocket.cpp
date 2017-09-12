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

#include "quicknet_udpsocket.h"
#include "quicknet_address.h" 
#include "quicknet_log.h"

namespace quicknet
{

#ifndef _WIN32
	// for linux
#	define INVALID_SOCKET ( (int32_t)-1 )
#	define SOCKET_ERROR   ( (int32_t)-1 )

#	include <errno.h>

// socket option parameter pointer type
#	define sockoptpp const void*
#else
	// socket option parameter pointer type
#	define sockoptpp const char*

// Windows requires to start WSA before any socket operation
	static bool s_networkInitialized = false;
	static void initializeNetwork()
	{
		if (!s_networkInitialized)
		{
			WORD wVersionRequested = MAKEWORD(2, 2);
			WSADATA wsaData;
			WSAStartup(wVersionRequested, &wsaData);
			s_networkInitialized = true;
		}
	}
#endif

	static const uint32_t s_defaultMTU = 1400;
	// TODO: set these to (lower) good values
	static const int32_t s_receiveBufferSize = 256 * 1024;
	static const int32_t s_sendBufferSize = 256 * 1024;

	UDPSocket::UDPSocket(RawSocket udpSocket)
	{
#ifdef _WIN32
		initializeNetwork();
#endif
		this->m_udpSocket = udpSocket;
	}

	UDPSocket::UDPSocket(bool isIPv6, int32_t timeout)
	{
#ifdef _WIN32
		initializeNetwork();
#endif
		m_udpSocket = socket(isIPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_udpSocket == INVALID_SOCKET)
		{
			return;
		}

		// this is crucial
		this->setBlockingMode(false);

		setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVBUF, (sockoptpp)&s_receiveBufferSize, sizeof(int32_t));
		setsockopt(m_udpSocket, SOL_SOCKET, SO_SNDBUF, (sockoptpp)&s_sendBufferSize, sizeof(int32_t));

		// this can be handy but not crucial
		int32_t so_reuse = 1;
		setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR, (sockoptpp)&so_reuse, sizeof(so_reuse));

		if (timeout > 0)
		{
			if (!this->SetTimeout(timeout))
			{
				this->Close();
				return;
			}
		}
	}

	UDPSocket::~UDPSocket()
	{
		this->Close();
	}

	bool UDPSocket::Close()
	{
		if (m_udpSocket != INVALID_SOCKET)
		{
#ifdef _WIN32
			closesocket(m_udpSocket);
#else
			close(m_udpSocket);
#endif
			m_udpSocket = INVALID_SOCKET;
			return true;
		}
		return false;
	}

	bool UDPSocket::Bind(const Address& address)
	{
		if (!this->IsValid()) { return false; }

		int32_t success = bind(m_udpSocket, &address.SockAddrc(), sizeof(address.SockAddrc()));

		return !(success < 0);
	}

	bool UDPSocket::Send(const Address& remote, uint8_t* data, uint32_t dataLength, uint32_t* bytesSent)
	{
		if (!this->IsValid() || (data == nullptr)) { return false; }

		if (dataLength >= s_defaultMTU)
		{
			Log::Warn("Trying to send a buffer bigger than MTU");
		}

		int32_t result = sendto(m_udpSocket, (const char*)data, dataLength, 0, &remote.SockAddrc(), sizeof(remote.SockAddrc()));
		if (result == SOCKET_ERROR)
		{
			return false;
		}
		*bytesSent = result;
		return true;
	}

	bool UDPSocket::Recv(uint8_t* buffer, uint32_t bufferSize, uint32_t* bytesRead, Address& remote)
	{
		if (!this->IsValid() || (buffer == nullptr)) { return false; }

		*bytesRead = 0;

#ifdef _WIN32
		int32_t remoteSize;
#else
		socklen_t remoteSize;
#endif	
		remoteSize = sizeof(remote.SockAddr());
		int32_t result = recvfrom(m_udpSocket, (char*)buffer, bufferSize, 0, &remote.SockAddr(), &remoteSize);
		if (result == SOCKET_ERROR)
		{
			int32_t error = this->getLastNetworkError();
			return (error == EWOULDBLOCK || error == EAGAIN);
		}

		*bytesRead = result;
		return true;
	}

	bool UDPSocket::SetTimeout(int32_t timeout)
	{
		if (!this->IsValid()) { return false; }

#ifdef _WIN32
		int32_t success = setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, (sockoptpp)&timeout, sizeof(int32_t));
#else
		struct timeval timeoutv;
		timeoutv.tv_sec = 0;
		timeoutv.tv_usec = timeout * 1000;
		int32_t success = setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVTIMEO, (sockoptpp)&timeoutv, sizeof(timeout));
#endif
		return (success == SOCKET_ERROR);
	}

	bool UDPSocket::AllowBroadcast(bool allow)
	{
		// TODO: set INADDR_BROADCAST? Not required in Windows at least
		if (!this->IsValid()) { return false; }

		uint32_t enable = allow ? 1 : 0;
		int32_t success = setsockopt(m_udpSocket, SOL_SOCKET, SO_BROADCAST, (sockoptpp)&enable, sizeof(int32_t));
		return (success == SOCKET_ERROR);
	}

	bool UDPSocket::IsValid()
	{
		return m_udpSocket != INVALID_SOCKET;
	}

	bool UDPSocket::setBlockingMode(bool blocking)
	{
		if (!this->IsValid()) { return false; }

#ifdef _WIN32
		uint64_t mode = blocking ? 0 : 1;
		int result = ioctlsocket(m_udpSocket, FIONBIO, (u_long*)&mode);
#else
		int flags = fcntl(m_udpSocket, F_GETFL, 0);
		if (flags < 0) return false;
		flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
		int result = fcntl(m_udpSocket, F_SETFL, flags);
#endif
		return result == 0;
	}

	uint32_t UDPSocket::getLastNetworkError()
	{
#ifdef _WIN32
		return WSAGetLastError();
#else
		return errno;
#endif
	}
}