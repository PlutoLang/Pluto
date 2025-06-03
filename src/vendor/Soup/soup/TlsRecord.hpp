#pragma once

#include "Packet.hpp"

#include "TlsContentType.hpp"
#include "TlsProtocolVersion.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsRecord)
	{
		TlsContentType_t content_type;
		TlsProtocolVersion version{};
		uint16_t length;

		SOUP_PACKET_IO(s) // 5 bytes
		{
			return s.u8(content_type)
				&& version.io(s)
				&& s.u16_be(length)
				;
		}
	};
}
