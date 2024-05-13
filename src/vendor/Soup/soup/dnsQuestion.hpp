#pragma once

#include "Packet.hpp"

#include "dnsClass.hpp"
#include "dnsName.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(dnsQuestion)
	{
		dnsName name;
		u16 qtype;
		u16 qclass = DNS_IN;

		SOUP_PACKET_IO(s)
		{
			return name.io(s)
				&& s.u16_be(qtype)
				&& s.u16_be(qclass)
				;
		}
	};
}
