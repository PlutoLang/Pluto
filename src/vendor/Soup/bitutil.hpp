#pragma once

#include "base.hpp"

#include <vector>

namespace soup
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
			uint8_t bits = 0;
			while ((((size_t)1) << bits) < range_size)
			{
				++bits;
			}
			return bits;
		}

		[[nodiscard]] static unsigned int getLeastSignificantSetBit(unsigned int mask) // UB if mask = 0
		{
			// These intrinsic functions just use the bsf instruction.
#if defined(_MSC_VER) && !defined(__clang__)
			unsigned long ret;
			_BitScanForward((unsigned long*)&ret, mask);
			return ret;
#else
			return __builtin_ctz(mask);
#endif
		}

		[[nodiscard]] static std::vector<bool> interleave(const std::vector<std::vector<bool>>& data); // assumes that all inner vectors have the same size
	};
}
