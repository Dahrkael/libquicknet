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

#include "quicknet_address.h"
#include <stdint.h>
#include <memory.h>
#include <sstream>

// to keep things cross-platform
#ifdef _WIN32
#	include <tchar.h>
#	define s_addr S_un.S_addr
#endif

namespace quicknet
{
	size_t NetAddressHasher::operator()(const Address& address) const
	{
		const struct sockaddr_in& sai = address.SockAddrInc();
		return sai.sin_addr.s_addr ^ sai.sin_port;
	}

	Address::Address()
	{
		memset(&m_inAddress.ss, 0, sizeof(struct sockaddr_storage));
	}

	Address::Address(const std::string& address, unsigned short port, bool isIPv6)
	{
		memset(&m_inAddress.ss, 0, sizeof(struct sockaddr_storage));

		// if its compiled as unicode, Windows requires wchar_t parameters
#if defined(_WIN32) && UNICODE
		std::wstring waddress;
		waddress.assign(address.begin(), address.end());
		const wchar_t* paddress = waddress.c_str();
#else
		const char* paddress = address.c_str();
#endif

		if (!isIPv6)
		{
			m_inAddress.s4.sin_family = AF_INET;
			m_inAddress.s4.sin_port = htons(port);

#ifdef _WIN32
			InetPton(AF_INET, paddress, &m_inAddress.s4.sin_addr);
#else
			inet_pton(AF_INET, paddress, &m_inAddress.s4.sin_addr);
#endif
		}
		else
		{
			m_inAddress.s6.sin6_family = AF_INET6;
			m_inAddress.s6.sin6_port = htons(port);
#ifdef _WIN32
			InetPton(AF_INET6, paddress, &m_inAddress.s6.sin6_addr);
#else
			inet_pton(AF_INET6, paddress, &m_inAddress.s4.sin_addr);
#endif
		}
	}

	Address::~Address()
	{
	}

	sockaddr& Address::SockAddr()
	{
		return m_inAddress.sa;
	}

	sockaddr_in& Address::SockAddrIn()
	{
		return m_inAddress.s4;
	}

	sockaddr_in6& Address::SockAddrIn6()
	{
		return m_inAddress.s6;
	}

	sockaddr_storage& Address::SockAddrStg()
	{
		return m_inAddress.ss;
	}

	const sockaddr& Address::SockAddrc() const
	{
		return m_inAddress.sa;
	}

	const sockaddr_in& Address::SockAddrInc() const
	{
		return m_inAddress.s4;
	}

	const sockaddr_in6& Address::SockAddrIn6c() const
	{
		return m_inAddress.s6;
	}

	const sockaddr_storage& Address::SockAddrStgc() const
	{
		return m_inAddress.ss;
	}

	std::string Address::ToIPv4String() const
	{
		uint32_t clientIP = m_inAddress.s4.sin_addr.s_addr;
		std::ostringstream stream;
		stream << (clientIP & 0xFF) << ".";
		stream << ((clientIP >> 8) & 0xFF) << ".";
		stream << ((clientIP >> 16) & 0xFF) << ".";
		stream << ((clientIP >> 24) & 0xFF) << ":";
		stream << ntohs(m_inAddress.s4.sin_port);

		return stream.str();
	}

	bool Address::operator==(Address& other) const {
		return SockAddrInc().sin_addr.s_addr == other.SockAddrInc().sin_addr.s_addr && SockAddrInc().sin_port == other.SockAddrInc().sin_port;
	}

	bool Address::operator!=(Address& other) const {
		return SockAddrInc().sin_addr.s_addr != other.SockAddrInc().sin_addr.s_addr || SockAddrInc().sin_port != other.SockAddrInc().sin_port;
	}

	bool Address::operator==(const Address& other) const {
		return SockAddrInc().sin_addr.s_addr == other.SockAddrInc().sin_addr.s_addr && SockAddrInc().sin_port == other.SockAddrInc().sin_port;
	}

	bool Address::operator!=(const Address& other) const {
		return SockAddrInc().sin_addr.s_addr != other.SockAddrInc().sin_addr.s_addr || SockAddrInc().sin_port != other.SockAddrInc().sin_port;
	}

	bool Address::operator<(const Address& other) const
	{
		uint64_t myaddr = SockAddrInc().sin_addr.s_addr;
		uint64_t otaddr = other.SockAddrInc().sin_addr.s_addr;

		if (myaddr < otaddr)
		{
			return true;
		}
		else
		{
			if (myaddr == otaddr)
			{
				if (SockAddrInc().sin_port < other.SockAddrInc().sin_port)
				{
					return true;
				}
			}
		}
		return false;
	}
}