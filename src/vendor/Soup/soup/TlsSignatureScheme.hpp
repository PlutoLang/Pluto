#pragma once

#include <cstdint>

#include "base.hpp"
#include "scoped_enum.hpp"

NAMESPACE_SOUP
{
	SCOPED_ENUM(TlsSignatureScheme, uint16_t,
		rsa_pkcs1_sha1 = 0x0201,
		ecdsa_sha1 = 0x0203,
		rsa_pkcs1_sha256 = 0x0401,
		ecdsa_secp256r1_sha256 = 0x0403,
		ecdsa_secp384r1_sha384 = 0x0503,
	);
}
