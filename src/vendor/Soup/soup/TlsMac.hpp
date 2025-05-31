#pragma once

#include "TlsRecord.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsMac)
	{
		u64 seq_num;
		TlsRecord record{};

		SOUP_PACKET_IO(s)
		{
			return s.u64_be(seq_num)
				&& record.io(s)
				;
		}
	};
}
