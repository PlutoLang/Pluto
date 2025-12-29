#include "Rc4State.hpp"

#include <utility> // std::swap

NAMESPACE_SOUP
{
	Rc4State::Rc4State(const uint8_t* key, uint8_t key_size) noexcept
	{
		{
			uint8_t i = 0;
			do {
				S[i] = i;
			} while (++i != 0);
		}
		{
			uint8_t i = 0;
			uint8_t j = 0;
			do {
				j = (j + S[i] + key[i % key_size]); // % 256
				std::swap(S[i], S[j]);
			} while (++i != 0);
		}
	}

	void Rc4State::transform(uint8_t* data, size_t size) noexcept
	{
		for (; size--; ++data)
		{
			i += 1;
			j += S[i];
			std::swap(S[i], S[j]);
			uint8_t t = (S[i] + S[j]); // % 256
			*data ^= S[t];
		}
	}
}
