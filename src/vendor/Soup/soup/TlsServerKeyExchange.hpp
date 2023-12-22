#pragma once

#include "Packet.hpp"

namespace soup
{
	SOUP_PACKET(TlsServerKeyExchange)
	{
		u8 curve_type;
		u16 named_curve;
		std::string point;

		SOUP_PACKET_IO(s)
		{
			return s.u8(curve_type)
				&& curve_type == 3
				&& s.u16(named_curve)
				&& s.template str_lp<u8_t>(point)
				;
		}
	};
}
