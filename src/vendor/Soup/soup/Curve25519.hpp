#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	// Original source: https://www.dlbeer.co.nz/oss/c25519.html and https://github.com/DavyLandman/compact25519
	// Original licence: Dedicated to the public domain.

	struct Curve25519
	{
		static constexpr auto KEY_SIZE = 32;
		static constexpr auto SHARED_SIZE = 32;

		static void generatePrivate(uint8_t private_key[KEY_SIZE]); // Generates a prepared private key.
		static void prepare(uint8_t key[KEY_SIZE]);

		static void derivePublic(uint8_t public_key[KEY_SIZE], const uint8_t private_key[KEY_SIZE]); // private_key must be prepared
		static void x25519(uint8_t shared_secret[SHARED_SIZE], const uint8_t my_private_key[KEY_SIZE], const uint8_t their_public_key[KEY_SIZE]); // my_private_key must be prepared
	};
}
