#pragma once

#include "Packet.hpp"

namespace soup
{
	SOUP_PACKET(TlsEncryptedPreMasterSecret)
	{
		std::string data;

		SOUP_PACKET_IO(s)
		{
			return s.template str_lp<u16_t>(data);
		}
	};
}
