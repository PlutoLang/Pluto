#pragma once

#include "type.hpp"

#include "Packet.hpp"

#include "TlsHelloExtensions.hpp"
#include "TlsProtocolVersion.hpp"
#include "TlsRandom.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsClientHello)
	{
		TlsProtocolVersion version{};
		TlsRandom random;
		std::string session_id{};
		std::vector<TlsCipherSuite_t> cipher_suites{};
		std::vector<uint8_t> compression_methods{};
		TlsHelloExtensions extensions{};

		SOUP_PACKET_IO(s)
		{
			return version.io(s)
				&& random.io(s)
				&& s.template str_lp<u8_t>(session_id, 32)
				&& s.vec_u16_bl_u16_be(cipher_suites)
				&& s.vec_u8_u8(compression_methods)
				&& extensions.io(s)
				;
		}
	};
}
