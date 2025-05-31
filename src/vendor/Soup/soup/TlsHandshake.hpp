#pragma once

#include "Packet.hpp"
#include "type.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsHandshake)
	{
		enum TlsHandshakeType : TlsHandshakeType_t
		{
			hello_request = 0,
			client_hello = 1,
			server_hello = 2,
			certificate = 11,
			server_key_exchange = 12,
			certificate_request = 13,
			server_hello_done = 14,
			certificate_verify = 15,
			client_key_exchange = 16,
			finished = 20,
		};

		u8 handshake_type;
		u24 length;

		SOUP_PACKET_IO(s)
		{
			return s.u8(handshake_type)
				&& s.u24_be(length)
				;
		}
	};
}
