#pragma once

#include "base.hpp"

#include <cstddef> // size_t
#include <cstdint>

NAMESPACE_SOUP
{
	struct Rc4State
	{
		uint8_t S[256];
		uint8_t i = 0;
		uint8_t j = 0;

		Rc4State(const uint8_t* key, uint8_t key_size) noexcept;

		template <typename T>
		Rc4State(const T& key) noexcept
			: Rc4State((const uint8_t*)key.data(), key.size() > 0xff ? 0xff : (uint8_t)key.size())
		{
		}

		// Encrypt or decrypt a block of data
		void transform(uint8_t* data, size_t size) noexcept;
	};
}
