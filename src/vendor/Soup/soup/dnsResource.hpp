#pragma once

#include "Packet.hpp"

#include "dnsName.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(dnsResource)
	{
		dnsName name;
		u16 rtype;
		u16 rclass;
		u32 ttl;
		std::string rdata;

		SOUP_PACKET_IO(s)
		{
			return name.io(s)
				&& s.u16be(rtype)
				&& s.u16be(rclass)
				&& s.u32be(ttl)
				&& s.template str_lp<u16be_t>(rdata)
				;
		}
	};
}
