#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum Asn1Type : uint32_t
	{
		ASN1_BOOLEAN = 1,
		ASN1_INTEGER = 2,
		ASN1_BITSTRING = 3,
		ASN1_STRING_OCTET = 4,
		ASN1_NULL = 5,
		ASN1_OID = 6,
		ASN1_UTF8STRING = 12,
		ASN1_SEQUENCE = 16,
		ASN1_SET = 17,
		ASN1_STRING_PRINTABLE = 19,
		ASN1_STRING_IA5 = 22,
		ASN1_UTCTIME = 23,
	};
}
