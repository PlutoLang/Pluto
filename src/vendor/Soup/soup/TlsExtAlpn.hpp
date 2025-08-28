#pragma once

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsExtAlpn)
	{
		std::vector<std::string> protocol_names;

		SOUP_PACKET_IO(s)
		{
			return s.vec_str_lp_u8_bl_u16_be(protocol_names);
		}
	};
}
