#pragma once

#include "type.hpp"

#include "Packet.hpp"

#include "TlsHelloExtensions.hpp"
#include "TlsProtocolVersion.hpp"
#include "TlsRandom.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsServerHello)
	{
		TlsProtocolVersion version{};
		TlsRandom random;
		std::string session_id{};
		TlsCipherSuite_t cipher_suite;
		u8 compression_method;
		TlsHelloExtensions extensions{};

		SOUP_PACKET_IO(s)
		{
			return version.io(s)
				&& random.io(s)
				&& s.template str_lp<u8_t>(session_id, 32)
				&& s.u16_be(cipher_suite)
				&& s.u8(compression_method)
				&& extensions.io(s)
				;
		}
	};
}
