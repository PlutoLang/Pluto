#include "Reader.hpp"

#if SOUP_X86 && SOUP_BITS == 64
	#include <emmintrin.h> // _mm_movemask_epi8
	#include <immintrin.h> // _pext_u64

	#include "bitutil.hpp"
	#include "CpuInfo.hpp"
#endif
#include "Endian.hpp"

NAMESPACE_SOUP
{
#if SOUP_X86 && SOUP_BITS == 64
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("sse4.1,bmi2")))
	#endif
	// _mm_extract_epi64 requires SSE4.1 & _pext_u64 requires BMI2.
	static void u64_dyn_decode_intrin(__m128i e, size_t byte_length, uint64_t& v)
	{
		const uint64_t mask = ((byte_length < 8) * (1ull << (8 * byte_length))) - 1;
		uint64_t lo = _pext_u64(_mm_cvtsi128_si64(e) & mask, 0x7f7f'7f7f'7f7f'7f7full);
		uint64_t hi = _mm_extract_epi64(e, 1) * (byte_length == 9);
		v = (hi << 56) | lo;
	}
#endif

	bool Reader::u64_dyn(uint64_t& v) noexcept
	{
#if SOUP_X86 && SOUP_BITS == 64
		if (CpuInfo::get().supportsSSE4_1() && CpuInfo::get().supportsBMI2())
		{
			const auto pos = getPosition();
			__m128i e;
			size_t read_bytes = 9;
			SOUP_RETHROW_FALSE(raw(&e, read_bytes) || (seekEnd(), (read_bytes = (getPosition() - pos)), seek(pos), raw(&e, read_bytes)));

			const auto byte_length = 1 + bitutil::getNumTrailingZeros(~static_cast<uint32_t>(_mm_movemask_epi8(e) & 0xff));
			u64_dyn_decode_intrin(e, byte_length, v);

			seek(pos + byte_length);
			return read_bytes >= byte_length;
		}
#endif
		v = 0;
		uint8_t b;
		uint8_t bits = 0;
		for (uint8_t i = 0; i != 8; ++i)
		{
			SOUP_RETHROW_FALSE(u8(b));
			v += (uint64_t)(b & 0x7f) << bits;
			if (!(b >> 7))
			{
				return true;
			}
			bits += 7;
		}
		SOUP_RETHROW_FALSE(u8(b));
		v += (uint64_t)b << 56;
		return true;
	}

#if SOUP_X86 && SOUP_BITS == 64
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("bmi2")))
	#endif
	uint64_t bmi2_u64_dyn_bias(size_t byte_length)
	{
		const auto biasbits = (byte_length >= 2) * (byte_length - 1);
		const auto biasmask = ((1u << biasbits) - 1u);
		return _pdep_u64(biasmask, 0x102040810204080ull);
	}
#endif

	bool Reader::u64_dyn_b(uint64_t& v) noexcept
	{
#if SOUP_X86 && SOUP_BITS == 64
		if (CpuInfo::get().supportsSSE4_1() && CpuInfo::get().supportsBMI2())
		{
			const auto pos = getPosition();
			__m128i e;
			size_t read_bytes = 9;
			SOUP_RETHROW_FALSE(raw(&e, read_bytes) || (seekEnd(), (read_bytes = (getPosition() - pos)), seek(pos), raw(&e, read_bytes)));

			const size_t byte_length = 1 + bitutil::getNumTrailingZeros(~static_cast<uint32_t>(_mm_movemask_epi8(e) & 0xff));
			u64_dyn_decode_intrin(e, byte_length, v);

			const uint64_t bias = bmi2_u64_dyn_bias(byte_length);
			bool valid = v <= 0xffffffffffffffff - bias;
			v += bias;

			seek(pos + byte_length);
			valid &= read_bytes >= byte_length;
			return valid;
		}
#endif
		v = 0;
		uint8_t b;
		uint8_t bits = 0;
		uint64_t bias = 0;
		for (uint8_t i = 0; i != 8; ++i)
		{
			SOUP_RETHROW_FALSE(u8(b));
			v += (uint64_t)(b & 0x7f) << bits;
			if (!(b >> 7))
			{
				goto _apply_bias;
			}
			bits += 7;
			bias += (uint64_t)1 << bits;
		}
		SOUP_RETHROW_FALSE(u8(b));
		v += (uint64_t)b << 56;
	_apply_bias:
		bool valid = v <= 0xffffffffffffffff - bias;
		v += bias;
		return valid;
	}

	bool Reader::u64_dyn_p(uint64_t& v) noexcept
	{
		uint8_t first_byte;
		SOUP_RETHROW_FALSE(u8(first_byte));
		const auto byte_length = 1 + (bitutil::getNumLeadingZeros(static_cast<uint32_t>((uint8_t)~first_byte)) - 24);
		const auto first_byte_value_bits = (byte_length < 8) * (8 - byte_length);
		v = 0;
		SOUP_RETHROW_FALSE(raw(&v, byte_length - 1));
		if constexpr (ENDIAN_NATIVE != ENDIAN_LITTLE)
		{
			v = Endianness::invert(v); static_assert(ENDIAN_NATIVE == ENDIAN_LITTLE || ENDIAN_NATIVE == ENDIAN_BIG);
		}
		v <<= first_byte_value_bits;
		v |= (first_byte & ((1 << first_byte_value_bits) - 1));
		return true;
	}

	bool Reader::u64_dyn_bp(uint64_t& v) noexcept
	{
		uint8_t first_byte;
		SOUP_RETHROW_FALSE(u8(first_byte));
		const auto byte_length = 1 + (bitutil::getNumLeadingZeros(static_cast<uint32_t>((uint8_t)~first_byte)) - 24);
		const auto first_byte_value_bits = (byte_length < 8) * (8 - byte_length);
		v = 0;
		SOUP_RETHROW_FALSE(raw(&v, byte_length - 1));
		if constexpr (ENDIAN_NATIVE != ENDIAN_LITTLE)
		{
			v = Endianness::invert(v); static_assert(ENDIAN_NATIVE == ENDIAN_LITTLE || ENDIAN_NATIVE == ENDIAN_BIG);
		}
		v <<= first_byte_value_bits;
		v |= (first_byte & ((1 << first_byte_value_bits) - 1));

		const auto biasbits = (byte_length >= 2) * (byte_length - 1);
		const uint64_t biasmask = ((1u << biasbits) - 1u);
		const auto bias = bitutil::parallelDeposit(biasmask, 0x102040810204080ull);

		bool valid = v <= 0xffffffffffffffff - bias;
		v += bias;
		return valid;
	}

	bool Reader::i64_dyn_a(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		v = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		if (neg)
		{
			v = static_cast<int64_t>(~(v - 1) | (static_cast<uint64_t>(1) << 63));
		}
		return true;
	}

	bool Reader::i64_dyn_b(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn_b(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		u = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		v = u ^ (0xffffffffffffffff * neg);
		return true;
	}

	bool Reader::i64_dyn_p(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn_p(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		u = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		v = u ^ (0xffffffffffffffff * neg);
		return true;
	}

	bool Reader::i64_dyn_bp(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn_bp(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		u = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		v = u ^ (0xffffffffffffffff * neg);
		return true;
	}

#if SOUP_X86 && SOUP_BITS == 64
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("bmi2")))
	#endif
	bool Reader::oml(uint32_t& v) noexcept
	{
		if (CpuInfo::get().supportsBMI2())
		{
			const auto pos = getPosition();
			uint64_t e;
			size_t read_bytes = 5;
			SOUP_RETHROW_FALSE(raw(&e, read_bytes) || (seekEnd(), (read_bytes = (getPosition() - pos)), seek(pos), raw(&e, read_bytes)));

			const uint32_t contbits = _pext_u32(static_cast<uint32_t>(e), 0x8080'8080ull);
			const auto byte_length = 1 + bitutil::getNumTrailingZeros(~contbits);

			const uint64_t mask = (1ull << (8 * byte_length)) - 1;
			v = static_cast<uint32_t>(_pext_u64(e & mask, 0x7f'7f7f'7f7full));

			seek(pos + byte_length);
			return read_bytes >= byte_length;
		}
		return oml<uint32_t>(v);
	}

	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("sse4.1,bmi2")))
	#endif
	bool Reader::oml(uint64_t& v) noexcept
	{
		if (CpuInfo::get().supportsSSE4_1() // _mm_extract_epi64
			&& CpuInfo::get().supportsBMI2() // _pext_u64
			)
		{
			const auto pos = getPosition();
			__m128i e;
			size_t read_bytes = 10;
			SOUP_RETHROW_FALSE(raw(&e, read_bytes) || (seekEnd(), (read_bytes = (getPosition() - pos)), seek(pos), raw(&e, read_bytes)));

			const uint32_t contbits = _mm_movemask_epi8(e) & 0x1ff;
			const auto byte_length = 1 + bitutil::getNumTrailingZeros(~contbits);

			const uint64_t mask_lo = ((byte_length < sizeof(uint64_t)) * (1ull << (8 * byte_length))) - 1;
			const uint64_t mask_hi = (byte_length > sizeof(uint64_t)) * ((1ull << (8 * (byte_length - sizeof(uint64_t)))) - 1);
			uint64_t lo = _pext_u64(_mm_cvtsi128_si64(e) & mask_lo, 0x7f7f'7f7f'7f7f'7f7full);
			uint64_t hi = _pext_u64(_mm_extract_epi64(e, 1) & mask_hi, 0x7f7f'7f7f'7f7f'7f7full);
			v = (hi << 56) | lo;

			seek(pos + byte_length);
			return read_bytes >= byte_length;
		}
		return oml<uint64_t>(v);
	}
#endif
}
