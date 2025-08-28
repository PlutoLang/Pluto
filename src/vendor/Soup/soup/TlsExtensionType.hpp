#pragma once

#include <cstdint>

#include "base.hpp"
#include "scoped_enum.hpp"

NAMESPACE_SOUP
{
	SCOPED_ENUM(TlsExtensionType, uint16_t,
		server_name = 0,
		elliptic_curves = 10,
		ec_point_formats = 11,
		signature_algorithms = 13,
		application_layer_protocol_negotiation = 16,
		extended_master_secret = 23,
		supported_versions = 43,
	);

	[[nodiscard]] inline bool tls_isGreaseyExtension(TlsExtensionType_t ext)
	{
		switch (ext)
		{
		case 0x0A0A:
		case 0x1A1A:
		case 0x2A2A:
		case 0x3A3A:
		case 0x4A4A:
		case 0x5A5A:
		case 0x6A6A:
		case 0x7A7A:
		case 0x8A8A:
		case 0x9A9A:
		case 0xAAAA:
		case 0xBABA:
		case 0xCACA:
		case 0xDADA:
		case 0xEAEA:
		case 0xFAFA:
			return true;
		}
		return false;
	}
}
