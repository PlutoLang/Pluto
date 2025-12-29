#include "Writer.hpp"

#if SOUP_X86 && SOUP_BITS == 64
#include <immintrin.h> // _pdep_u64

#include "CpuInfo.hpp"
#endif
#include "bitutil.hpp"
#include "Endian.hpp"

NAMESPACE_SOUP
{
#if SOUP_X86 && SOUP_BITS == 64
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("bmi2")))
	#endif
	static void bmi2_u64_dyn_encode(uint64_t v, size_t byte_length, uint64_t e[2])
	{
		const uint64_t mask = ((byte_length < 9) * (1ull << (8 * (byte_length - 1)))) - 1;
		const uint64_t contbits = 0x8080'8080'8080'8080ull & mask;
		e[0] = _pdep_u64(v, 0x7f7f'7f7f'7f7f'7f7full) | contbits;
		e[1] = v >> 56;
	}
#endif

	bool Writer::u64_dyn(const uint64_t& v) noexcept
	{
#if SOUP_X86 && SOUP_BITS == 64
		if (CpuInfo::get().supportsBMI2())
		{
			static constexpr uint64_t smallest_value_needing_2_bytes_to_encode = 1ull << 7;
			static constexpr uint64_t smallest_value_needing_3_bytes_to_encode = 1ull << 14;
			static constexpr uint64_t smallest_value_needing_4_bytes_to_encode = 1ull << 21;
			static constexpr uint64_t smallest_value_needing_5_bytes_to_encode = 1ull << 28;
			static constexpr uint64_t smallest_value_needing_6_bytes_to_encode = 1ull << 35;
			static constexpr uint64_t smallest_value_needing_7_bytes_to_encode = 1ull << 42;
			static constexpr uint64_t smallest_value_needing_8_bytes_to_encode = 1ull << 49;
			static constexpr uint64_t smallest_value_needing_9_bytes_to_encode = 1ull << 56;

			const size_t byte_length = 1
				+ (v >= smallest_value_needing_2_bytes_to_encode)
				+ (v >= smallest_value_needing_3_bytes_to_encode)
				+ (v >= smallest_value_needing_4_bytes_to_encode)
				+ (v >= smallest_value_needing_5_bytes_to_encode)
				+ (v >= smallest_value_needing_6_bytes_to_encode)
				+ (v >= smallest_value_needing_7_bytes_to_encode)
				+ (v >= smallest_value_needing_8_bytes_to_encode)
				+ (v >= smallest_value_needing_9_bytes_to_encode)
				;

			uint64_t e[2];
			bmi2_u64_dyn_encode(v, byte_length, e);

			return raw(e, byte_length);
		}
#endif
		bool ret = true;
		uint64_t in = v;
		uint8_t cur;
		for (uint8_t i = 0; i != 8; ++i)
		{
			cur = (in & 0x7f);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				ret &= u8(cur);
			}
			else
			{
				ret &= u8(cur);
				return ret;
			}
		}
		cur = (uint8_t)in;
		ret &= u8(cur);
		return ret;
	}

#if SOUP_X86 && SOUP_BITS == 64
	uint64_t bmi2_u64_dyn_bias(size_t byte_length);
#endif

	bool Writer::u64_dyn_b(const uint64_t& v) noexcept
	{
#if SOUP_X86 && SOUP_BITS == 64
		if (CpuInfo::get().supportsBMI2())
		{
			static constexpr uint64_t smallest_value_needing_2_bytes_to_encode = 128ull;
			static constexpr uint64_t smallest_value_needing_3_bytes_to_encode = 16512ull;
			static constexpr uint64_t smallest_value_needing_4_bytes_to_encode = 2113664ull;
			static constexpr uint64_t smallest_value_needing_5_bytes_to_encode = 270549120ull;
			static constexpr uint64_t smallest_value_needing_6_bytes_to_encode = 34630287488ull;
			static constexpr uint64_t smallest_value_needing_7_bytes_to_encode = 4432676798592ull;
			static constexpr uint64_t smallest_value_needing_8_bytes_to_encode = 567382630219904ull;
			static constexpr uint64_t smallest_value_needing_9_bytes_to_encode = 72624976668147840ull;

			const size_t byte_length = 1
				+ (v >= smallest_value_needing_2_bytes_to_encode)
				+ (v >= smallest_value_needing_3_bytes_to_encode)
				+ (v >= smallest_value_needing_4_bytes_to_encode)
				+ (v >= smallest_value_needing_5_bytes_to_encode)
				+ (v >= smallest_value_needing_6_bytes_to_encode)
				+ (v >= smallest_value_needing_7_bytes_to_encode)
				+ (v >= smallest_value_needing_8_bytes_to_encode)
				+ (v >= smallest_value_needing_9_bytes_to_encode)
				;

			uint64_t e[2];
			bmi2_u64_dyn_encode(v - bmi2_u64_dyn_bias(byte_length), byte_length, e);

			return raw(e, byte_length);
		}
#endif
		bool ret = true;
		uint64_t in = v;
		uint8_t cur;
		for (uint8_t i = 0; i != 8; ++i)
		{
			cur = (in & 0x7f);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				ret &= u8(cur);
				in -= 1; // bias
			}
			else
			{
				ret &= u8(cur);
				return ret;
			}
		}
		cur = (uint8_t)in;
		ret &= u8(cur);
		return ret;
	}

	bool Writer::u64_dyn_p(const uint64_t& v) noexcept
	{
		static constexpr uint64_t smallest_value_needing_2_bytes_to_encode = 1ull << 7;
		static constexpr uint64_t smallest_value_needing_3_bytes_to_encode = 1ull << 14;
		static constexpr uint64_t smallest_value_needing_4_bytes_to_encode = 1ull << 21;
		static constexpr uint64_t smallest_value_needing_5_bytes_to_encode = 1ull << 28;
		static constexpr uint64_t smallest_value_needing_6_bytes_to_encode = 1ull << 35;
		static constexpr uint64_t smallest_value_needing_7_bytes_to_encode = 1ull << 42;
		static constexpr uint64_t smallest_value_needing_8_bytes_to_encode = 1ull << 49;
		static constexpr uint64_t smallest_value_needing_9_bytes_to_encode = 1ull << 56;

		const auto byte_length = 1
			+ (v >= smallest_value_needing_2_bytes_to_encode)
			+ (v >= smallest_value_needing_3_bytes_to_encode)
			+ (v >= smallest_value_needing_4_bytes_to_encode)
			+ (v >= smallest_value_needing_5_bytes_to_encode)
			+ (v >= smallest_value_needing_6_bytes_to_encode)
			+ (v >= smallest_value_needing_7_bytes_to_encode)
			+ (v >= smallest_value_needing_8_bytes_to_encode)
			+ (v >= smallest_value_needing_9_bytes_to_encode)
			;

		const auto first_byte_value_bits = (byte_length < 8) * (8 - byte_length);
		const auto first_byte_prefix_bits = byte_length - 1;

		uint64_t w = v;

		uint8_t first_byte = (0xff << (8 - first_byte_prefix_bits)) | (w & ((1 << first_byte_value_bits) - 1));
		bool res = u8(first_byte);
		w >>= first_byte_value_bits;
		if constexpr (ENDIAN_NATIVE != ENDIAN_LITTLE)
		{
			w = Endianness::invert(w); static_assert(ENDIAN_NATIVE == ENDIAN_LITTLE || ENDIAN_NATIVE == ENDIAN_BIG);
		}
		res &= raw(&w, byte_length - 1);
		return res;
	}

	bool Writer::u64_dyn_bp(const uint64_t& v) noexcept
	{
		static constexpr uint64_t smallest_value_needing_2_bytes_to_encode = 128ull;
		static constexpr uint64_t smallest_value_needing_3_bytes_to_encode = 16512ull;
		static constexpr uint64_t smallest_value_needing_4_bytes_to_encode = 2113664ull;
		static constexpr uint64_t smallest_value_needing_5_bytes_to_encode = 270549120ull;
		static constexpr uint64_t smallest_value_needing_6_bytes_to_encode = 34630287488ull;
		static constexpr uint64_t smallest_value_needing_7_bytes_to_encode = 4432676798592ull;
		static constexpr uint64_t smallest_value_needing_8_bytes_to_encode = 567382630219904ull;
		static constexpr uint64_t smallest_value_needing_9_bytes_to_encode = 72624976668147840ull;

		const auto byte_length = 1
			+ (v >= smallest_value_needing_2_bytes_to_encode)
			+ (v >= smallest_value_needing_3_bytes_to_encode)
			+ (v >= smallest_value_needing_4_bytes_to_encode)
			+ (v >= smallest_value_needing_5_bytes_to_encode)
			+ (v >= smallest_value_needing_6_bytes_to_encode)
			+ (v >= smallest_value_needing_7_bytes_to_encode)
			+ (v >= smallest_value_needing_8_bytes_to_encode)
			+ (v >= smallest_value_needing_9_bytes_to_encode)
			;

		const auto first_byte_value_bits = (byte_length < 8) * (8 - byte_length);
		const auto first_byte_prefix_bits = byte_length - 1;

		const auto subbits = (byte_length >= 2) * (byte_length - 1);
		const uint64_t submask = ((1u << subbits) - 1u);
		uint64_t w = v - bitutil::parallelDeposit(submask, 0x102040810204080ull);

		uint8_t first_byte = (0xff << (8 - first_byte_prefix_bits)) | (w & ((1 << first_byte_value_bits) - 1));
		bool res = u8(first_byte);
		w >>= first_byte_value_bits;
		if constexpr (ENDIAN_NATIVE != ENDIAN_LITTLE)
		{
			w = Endianness::invert(w); static_assert(ENDIAN_NATIVE == ENDIAN_LITTLE || ENDIAN_NATIVE == ENDIAN_BIG);
		}
		res &= raw(&w, byte_length - 1);
		return res;
	}

	bool Writer::i64_dyn_a(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		if (neg)
		{
			u = (~v + 1) & ~((uint64_t)1 << 63);
		}
		else
		{
			u = v;
		}
		return u64_dyn(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
	}

	bool Writer::i64_dyn_b(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		u = v ^ (0xffffffffffffffff * neg);
		return u64_dyn_b(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
	}

	bool Writer::i64_dyn_p(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		u = v ^ (0xffffffffffffffff * neg);
		return u64_dyn_p(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
	}

	bool Writer::i64_dyn_bp(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		u = v ^ (0xffffffffffffffff * neg);
		return u64_dyn_bp(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
	}

	bool Writer::mysql_lenenc(const uint64_t& v) noexcept
	{
		if (v < 0xFB)
		{
			auto val = (uint8_t)v;
			return u8(val);
		}
		else if (v <= 0xFFFF)
		{
			uint8_t prefix = 0xFC;
			auto val = (uint16_t)v;
			return u8(prefix) && u16_le(val);
		}
		else if (v <= 0xFFFFFF)
		{
			uint8_t prefix = 0xFD;
			auto val = (uint32_t)v;
			return u8(prefix) && u24_le(val);
		}
		uint8_t prefix = 0xFE;
		auto val = v;
		return u8(prefix) && u64_le(val);
	}
}
