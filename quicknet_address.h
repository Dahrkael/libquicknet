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
#include <string>
#include "quicknet_includes.h"

namespace quicknet
{
	class Address
	{
	public:
		Address();
		Address(const std::string& address, unsigned short port, bool isIPv6 = false);
		~Address();

		struct sockaddr&		 SockAddr();
		struct sockaddr_in&		 SockAddrIn();
		struct sockaddr_in6&	 SockAddrIn6();
		struct sockaddr_storage& SockAddrStg();

		const struct sockaddr&			SockAddrc() const;
		const struct sockaddr_in&		SockAddrInc() const;
		const struct sockaddr_in6&		SockAddrIn6c() const;
		const struct sockaddr_storage&	SockAddrStgc() const;

		std::string ToIPv4String() const;
		bool operator==(Address& other) const;
		bool operator==(const Address& other) const;
		bool operator!=(Address& other) const;
		bool operator!=(const Address& other) const;
		bool operator<(const Address& other) const;

	private:
		union {
			struct sockaddr         sa;
			struct sockaddr_in      s4;
			struct sockaddr_in6     s6;
			struct sockaddr_storage ss;
		} m_inAddress;
	};

	struct NetAddressHasher {
		size_t operator()(const Address& address) const;
	};
}