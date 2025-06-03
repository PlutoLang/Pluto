#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(TlsServerECDHParams)
	{
		u8 curve_type;
		u16 named_curve;
		std::string point;

		SOUP_PACKET_IO(s)
		{
			return s.u8(curve_type)
				&& curve_type == 3
				&& s.u16_be(named_curve)
				&& s.template str_lp<u8_t>(point)
				;
		}
	};
	
	SOUP_PACKET(TlsServerKeyExchange)
	{
		TlsServerECDHParams params;

		u16 signature_scheme; // https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-signaturescheme
		std::string signature;

		SOUP_PACKET_IO(s)
		{
			return params.io(s)
				&& s.u16_be(signature_scheme)
				&& s.template str_lp<u16_be_t>(signature)
				;
		}
	};
}
