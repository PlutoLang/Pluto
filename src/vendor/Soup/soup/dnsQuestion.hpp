#pragma once

#include "Packet.hpp"

#include "dnsClass.hpp"
#include "dnsName.hpp"

namespace soup
{
	SOUP_PACKET(dnsQuestion)
	{
		dnsName name;
		u16 qtype;
		u16 qclass = DNS_IN;

		SOUP_PACKET_IO(s)
		{
			return name.io(s)
				&& s.u16(qtype)
				&& s.u16(qclass)
				;
		}
	};
}
