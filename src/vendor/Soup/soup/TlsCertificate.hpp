#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsCertificate)
	{
		std::vector<std::string> asn1_certs{};

		SOUP_PACKET_IO(s)
		{
			return s.vec_str_lp_u24_bl_u24_be(asn1_certs);
		}
	};
}
