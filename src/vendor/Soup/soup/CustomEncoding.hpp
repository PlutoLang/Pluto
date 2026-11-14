#pragma once

#include <cmath> // ceil
#include <cstring> // strlen
#include <string>

#include "base.hpp"

#include "alpha2decodetbl.hpp"
#include "Bigint.hpp"
#include "bitutil.hpp"
#include "StringLiteral.hpp"

NAMESPACE_SOUP
{
	static_assert(SOUP_CPP20, "soup::CustomEncoding requires C++ 20 or above");

	template <StringLiteral AlphaStr>
	struct CustomEncoding
	{
		static constexpr const char* ALPHA = AlphaStr.c_str();
		static inline const Bigint ALPHA_SIZE = AlphaStr.size();

		static inline const auto decodetbl = alpha2decodetbl(AlphaStr.c_str());

		// Note that this may be higher than the actual encoded length.
		[[nodiscard]] static size_t getEncodedLength(size_t inlen)
		{
			size_t alpha_size;
			SOUP_ASSERT(ALPHA_SIZE.toPrimitive(alpha_size));
			uint8_t seqlen = bitutil::getBitsNeededToEncodeRange(alpha_size);
			size_t bytelen = static_cast<size_t>(ceil(8.0f / seqlen));
			inlen *= bytelen;
			return static_cast<size_t>(ceil(((float)inlen * seqlen) / 8));
		}

		[[nodiscard]] static std::string encode(const std::string& msg) { return encode(msg.data(), msg.size()); }
		[[nodiscard]] static std::string encode(const char* data, size_t size) { return encode(Bigint::fromBinary(data, size)); }
		[[nodiscard]] static std::string encode(Bigint msg)
		{
			std::string enc{};
			while (!msg.isZero())
			{
				auto [q, r] = msg.divideUnsigned(ALPHA_SIZE);
				enc.insert(0, 1, ALPHA[r.getChunk(0)]);
				msg = std::move(q);
			}
			return enc;
		}

		[[nodiscard]] static std::string encodeWithPadding(const std::string& msg) { return encodeWithPadding(msg.data(), msg.size()); }
		[[nodiscard]] static std::string encodeWithPadding(const char* data, size_t size)
		{
			auto enc = encode(data, size);
			auto len = getEncodedLength(size);
			while (enc.size() < len)
			{
				enc.insert(0, 1, ALPHA[0]);
			}
			return enc;
		}

		// Note that leading zero-bytes will be trimmed both when encoding without padding, and by Bigint::toBinary during decoding, so if there is a certain length expectation, zeroes should be added to the front of the decoded data if that expectation is not met.
		[[nodiscard]] static std::string decode(const std::string& enc) { return decode(enc.data(), enc.size()); }
		[[nodiscard]] static std::string decode(const char* data, size_t size)
		{
			Bigint dec{};
			for (; size--; ++data)
			{
				dec *= ALPHA_SIZE;
				dec += (Bigint::chunk_t)decodetbl[*data];
			}
			return dec.toBinary();
		}
	};
}
