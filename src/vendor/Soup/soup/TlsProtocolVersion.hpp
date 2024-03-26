#pragma once

#include "Packet.hpp"

namespace soup
{
	struct TlsProtocolVersion
	{
		uint8_t major = 3;
		uint8_t minor = 3;

		SOUP_PACKET_IO(s) // 2 bytes
		{
			return s.u8(major)
				&& s.u8(minor)
				;
		}
	};
}
