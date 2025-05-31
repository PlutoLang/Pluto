#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsEncryptedPreMasterSecret)
	{
		std::string data;

		SOUP_PACKET_IO(s)
		{
			return s.template str_lp<u16_be_t>(data);
		}
	};
}
