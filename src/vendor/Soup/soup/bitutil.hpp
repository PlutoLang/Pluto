#pragma once

#include <cstddef> // size_t
#include <cstdint>
#include <vector>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct bitutil
	{
		template<typename OutContainer, int frombits, int tobits, typename InContainer>
		[[nodiscard]] static OutContainer msb_first(const InContainer& in)
		{
			OutContainer out;
			out.reserve((in.length() / tobits) * frombits);

			int acc = 0;
			int bits = 0;
			constexpr int maxv = (1 << tobits) - 1;
			constexpr int max_acc = (1 << (frombits + tobits - 1)) - 1;
			for (size_t i = 0; i < in.size(); ++i)
			{
				int value = (uint8_t)in[i];
				acc = ((acc << frombits) | value) & max_acc;
				bits += frombits;
				while (bits >= tobits)
				{
					bits -= tobits;
					out.push_back((acc >> bits) & maxv);
				}
			}
			//if (pad)
			//{
			if (bits)
			{
				out.push_back((acc << (tobits - bits)) & maxv);
			}
			//}
			//else if (bits >= frombits || ((acc << (tobits - bits)) & maxv))
			//{
			//	return false;
			//}
			return out;
		}

		[[nodiscard]] static constexpr uint8_t getBitsNeededToEncodeRange(size_t range_size) // aka. ceil(log2(range_size))
		{
#if SOUP_CPP20
			if (std::is_constant_evaluated()
#if SOUP_BITS > 32
				|| range_size > 0xFFFFFFFF
#endif
				)
			{
#endif
				uint8_t bits = 0;
				while ((((size_t)1) << bits) < range_size)
				{
					++bits;
				}
				return bits;
#if SOUP_CPP20
			}
			else
			{
				SOUP_IF_LIKELY (range_size > 1)
				{
					return getMostSignificantSetBit((uint32_t)range_size - 1) + 1;
				}
				return 0;
			}
#endif
		}

		[[nodiscard]] static unsigned long getLeastSignificantSetBit(uint16_t mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0); // UB!

			// These intrinsic functions just use the bsf instruction.
#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long ret;
			_BitScanForward(&ret, static_cast<uint32_t>(mask));
			return ret;
#else
			return __builtin_ctz(mask);
#endif
		}

		[[nodiscard]] static unsigned long getLeastSignificantSetBit(uint32_t mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0); // UB!

			// These intrinsic functions just use the bsf instruction.
#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long ret;
			_BitScanForward(&ret, mask);
			return ret;
#else
			return __builtin_ctz(mask);
#endif
		}

#if SOUP_BITS >= 64
		[[nodiscard]] static unsigned long getLeastSignificantSetBit(uint64_t mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0); // UB!

			// These intrinsic functions just use the bsf instruction.
#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long ret;
			_BitScanForward64(&ret, mask);
			return ret;
#else
			return __builtin_ctz(mask);
#endif
		}
#endif

		template <typename T>
		static constexpr void unsetLeastSignificantSetBit(T& val)
		{
			val &= (val - 1);
		}

		[[nodiscard]] static unsigned int getNumLeadingZeros(uint32_t mask) noexcept
		{
			unsigned int res = 32;
			if (mask != 0)
			{
#if defined(_MSC_VER) && !defined(__clang__)
				unsigned long ret;
				_BitScanReverse(&ret, mask);
				res -= ret;
#else
				res = __builtin_clz(mask);
#endif
			}
			return res;
		}

		[[nodiscard]] static unsigned int getMostSignificantSetBit(uint32_t mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0); // UB!

#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long ret;
			_BitScanReverse(&ret, mask);
			return ret;
#else
 			return 31 - __builtin_clz(mask);
#endif
		}

		[[nodiscard]] static uint32_t getNumSetBits(uint32_t i) noexcept
		{
#if defined(_MSC_VER) && !defined(__clang__)
			// https://stackoverflow.com/a/109025
			i = i - ((i >> 1) & 0x55555555);
			i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
			i = (i + (i >> 4)) & 0x0F0F0F0F;
			i *= 0x01010101;
			return i >> 24;
#else
			return __builtin_popcount(i);
#endif
		}

		// https://stackoverflow.com/a/2602885
		[[nodiscard]] static uint8_t reverse(uint8_t b) noexcept
		{
			b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
			b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
			b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
			return b;
		}

		[[nodiscard]] static std::vector<bool> interleave(const std::vector<std::vector<bool>>& data); // assumes that all inner vectors have the same size
	};
}
