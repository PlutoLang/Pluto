#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsClientHelloExtServerName)
	{
		std::string host_name;

		SOUP_PACKET_IO(s)
		{
			if (s.isRead())
			{
				uint16_t len = 0;
				if (s.u16_be(len) && len > 3)
				{
					uint8_t num_people_who_asked;
					if (s.u8(num_people_who_asked)
						&& num_people_who_asked == 0
						)
					{
						return s.template str_lp<u16_be_t>(host_name);
					}
				}
			}
			else if (s.isWrite())
			{
				uint16_t len = static_cast<uint16_t>(host_name.length() + 3);
				if (s.u16_be(len))
				{
					uint8_t num_people_who_asked = 0;
					return s.u8(num_people_who_asked)
						&& s.template str_lp<u16_be_t>(host_name)
						;
				}
			}
			return false;
		}
	};
}
