#pragma once

#include "Packet.hpp"

namespace soup
{
	SOUP_PACKET(dnsHeader)
	{
		u16 id;
		u8 bitfield1;
		u8 bitfield2;
		u16 qdcount;
		u16 ancount;
		u16 nscount;
		u16 arcount;

		SOUP_PACKET_IO(s)
		{
			return s.u16(id)
				&& s.u8(bitfield1)
				&& s.u8(bitfield2)
				&& s.u16(qdcount)
				&& s.u16(ancount)
				&& s.u16(nscount)
				&& s.u16(arcount)
				;
		}

		void setIsResponse(bool b) noexcept
		{
			bitfield1 &= ~(1 << 7);
			bitfield1 |= (b << 7);
		}

		[[nodiscard]] bool isResponse() const noexcept
		{
			return (bitfield1 >> 7) & 1;
		}

		void setRecursionDesired(bool b) noexcept
		{
			bitfield1 &= ~(1 << 0);
			bitfield1 |= (b << 0);
		}

		[[nodiscard]] bool isRecursionDesired() const noexcept
		{
			return (bitfield1 >> 0) & 1;
		}
	};
}
