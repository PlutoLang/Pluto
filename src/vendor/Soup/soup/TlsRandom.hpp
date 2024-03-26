#pragma once

#include "Packet.hpp"

namespace soup
{
	SOUP_PACKET(TlsRandom)
	{
		u32 time;
		u8 random[28];

		SOUP_PACKET_IO(s)
		{
			if (!s.u32(time))
			{
				return false;
			}
			for (auto& b : random)
			{
				if (!s.u8(b))
				{
					return false;
				}
			}
			return true;
		}
	};
	static_assert(sizeof(TlsRandom) == 32);
}
