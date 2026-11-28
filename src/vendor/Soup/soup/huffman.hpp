#pragma once

#include "base.hpp"

#include <cstdint>
#include <cstddef>

NAMESPACE_SOUP
{
	// Returns the greatest length.
	uint8_t huffman_lengths(uint8_t lengths[/*num_symbols*/], const size_t frequencies[/*num_symbols*/], uint_fast16_t num_symbols) SOUP_EXCAL;

	// Canonical Huffman codes (RFC 1951 3.2.2), returned bit-reversed.
	void huffman_codes(uint16_t codes[/*num_symbols*/], const uint8_t lengths[/*num_symbols*/], uint_fast16_t num_symbols) noexcept;
}
