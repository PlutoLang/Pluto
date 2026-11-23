#pragma once

#include <cstddef> // size_t
#include <cstdint>
#include <vector>

#include "bit.hpp" // popcount, countl_zero, countr_zero

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

		template <typename T>
		[[nodiscard]] static unsigned int getLeastSignificantSetBit(T mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0);
			return getNumTrailingZeros(mask);
		}

		template <typename T>
		[[nodiscard]] static unsigned int getNumTrailingZeros(T mask) noexcept
		{
			return soup::countr_zero<T>(mask);
		}

		template <typename T>
		static constexpr void unsetLeastSignificantSetBit(T& val)
		{
			val &= (val - 1);
		}

		template <typename T>
		[[nodiscard]] static unsigned int getNumLeadingZeros(T mask) noexcept
		{
			return soup::countl_zero<T>(mask);
		}

		[[nodiscard]] static unsigned int getMostSignificantSetBit(uint32_t mask) noexcept
		{
			SOUP_DEBUG_ASSERT(mask != 0); // UB!

#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long idx;
			_BitScanReverse(&idx, mask);
			return idx;
#else
			return 31 - __builtin_clz(mask);
#endif
		}

		template <typename T>
		[[nodiscard]] static auto getNumSetBits(const T val) noexcept
		{
			return soup::popcount<T>(val);
		}

		// https://stackoverflow.com/a/2602885
		// https://graphics.stanford.edu/%7Eseander/bithacks.html#BitReverseObvious
		// C++29 anyone? https://en.cppreference.com/cpp/numeric/bit_reverse
		[[nodiscard]] static uint8_t reverse(uint8_t v) noexcept
		{
			v = static_cast<uint8_t>(((v >> 1) & 0x55) | ((v & 0x55) << 1));
			v = static_cast<uint8_t>(((v >> 2) & 0x33) | ((v & 0x33) << 2));
			v = static_cast<uint8_t>((v >> 4) | (v << 4));
			return v;
		}
		[[nodiscard]] static uint16_t reverse(uint16_t v)
		{
			v = static_cast<uint16_t>(((v >> 1) & 0x5555) | ((v & 0x5555) << 1));
			v = static_cast<uint16_t>(((v >> 2) & 0x3333) | ((v & 0x3333) << 2));
			v = static_cast<uint16_t>(((v >> 4) & 0x0F0F) | ((v & 0x0F0F) << 4));
			v = static_cast<uint16_t>((v >> 8) | (v << 8));
			return v;
		}
		[[nodiscard]] static uint32_t reverse(uint32_t v)
		{
			v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
			v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
			v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
			v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
			v = (v >> 16) | (v << 16);
			return v;
		}
		template <typename T>
		[[nodiscard]] static T reverse(T v, uint_fast8_t bits)
		{
			T res = 0;
			for (uint_fast8_t i = 0; i != bits; ++i)
			{
				res = (res << 1) | (v & 1);
				v >>= 1;
			}
			return res;
		}

		[[nodiscard]] static uint32_t parallelDeposit(uint32_t src, uint32_t mask);
		[[nodiscard]] static uint64_t parallelDeposit(uint64_t src, uint64_t mask);

		[[nodiscard]] static std::vector<bool> interleave(const std::vector<std::vector<bool>>& data); // assumes that all inner vectors have the same size
	};
}
