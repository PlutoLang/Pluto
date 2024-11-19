#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsClientHelloExtEllipticCurves)
	{
		std::vector<uint16_t> named_curves;

		SOUP_PACKET_IO(s)
		{
			return s.vec_u16be_bl_u16be(named_curves);
		}
	};
}
