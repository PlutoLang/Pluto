#pragma once

#include "base.hpp"
#include "type.hpp" // halfsize_t, halfintmax_t

#ifndef SOUP_BIGINT_USE_INTVECTOR
#define SOUP_BIGINT_USE_INTVECTOR SOUP_WINDOWS
#endif

#ifndef SOUP_BIGINT_USE_INTRIN
#define SOUP_BIGINT_USE_INTRIN (SOUP_X86 && SOUP_BITS == 64)
#endif

#include <string>
#include <utility> // pair
#if !SOUP_BIGINT_USE_INTVECTOR
#include <vector>
#endif

#include "fwd.hpp"

#if SOUP_BIGINT_USE_INTRIN
#include <emmintrin.h>
#endif

#if SOUP_BIGINT_USE_INTVECTOR
#include "IntVector.hpp"
#endif
#include "Packet.hpp"
#include "stringifyable.hpp"
#include "Task.hpp"

NAMESPACE_SOUP
{
	class Bigint
	{
	public:
		using chunk_t = halfsize_t;
		using chunk_signed_t = halfintmax_t;

#if SOUP_BIGINT_USE_INTVECTOR
		IntVector<chunk_t> chunks{};
#else
		std::vector<chunk_t> chunks{};
#endif
		bool negative = false;

		Bigint() noexcept = default;
		Bigint(chunk_signed_t v) SOUP_EXCAL;
		Bigint(chunk_t v, bool negative = false) SOUP_EXCAL;
		Bigint(intmax_t v) SOUP_EXCAL;
		Bigint(size_t v, bool negative = false) SOUP_EXCAL;
		Bigint(Bigint&& b) noexcept;
		Bigint(const Bigint& b) SOUP_EXCAL;

		[[nodiscard]] static Bigint fromString(const std::string& str);
		[[nodiscard]] static Bigint fromString(const char* str, size_t len);
		[[nodiscard]] static Bigint fromStringHex(const char* str, size_t len);
	private:
		void fromStringImplBinary(const char* str, size_t len);
		void fromStringImplDecimal(const char* str, size_t len);
		void fromStringImplHex(const char* str, size_t len);

	public:
		[[nodiscard]] static Bigint random(size_t bits) SOUP_EXCAL;
		[[nodiscard]] static Bigint random(RngInterface& rng, size_t bits) SOUP_EXCAL;
		[[nodiscard]] static Bigint randomProbablePrime(const size_t bits, const int miller_rabin_iterations = 1) SOUP_EXCAL;
		[[nodiscard]] static Bigint randomProbablePrime(RngInterface& rng, const size_t bits, const int miller_rabin_iterations = 1) SOUP_EXCAL;

		[[nodiscard]] static constexpr uint8_t getBitsPerChunk() noexcept
		{
			return SOUP_BITS / 2;
		}

		[[nodiscard]] static constexpr uint8_t getBytesPerChunk() noexcept
		{
			return getBitsPerChunk() / 8;
		}

		[[nodiscard]] static constexpr uint8_t getNibblesPerChunk() noexcept
		{
			return getBytesPerChunk() * 2;
		}

		[[nodiscard]] static constexpr chunk_t getCarry(size_t v) noexcept
		{
			return (chunk_t)(v >> getBitsPerChunk());
		}

		[[nodiscard]] SOUP_CONSTEXPR20 size_t getNumChunks() const noexcept
		{
			return chunks.size();
		}

		[[nodiscard]] chunk_t getChunk(size_t i) const noexcept;
		[[nodiscard]] SOUP_FORCEINLINE chunk_t getChunkInbounds(size_t i) const noexcept { return chunks[i]; }
		void setChunk(size_t i, chunk_t v) SOUP_EXCAL;
		SOUP_FORCEINLINE void setChunkInbounds(size_t i, chunk_t v) noexcept { chunks[i] = v; }
		void addChunk(size_t i, chunk_t v) SOUP_EXCAL;
		void addChunk(chunk_t v) SOUP_EXCAL;
		void shrink() noexcept;

		[[nodiscard]] SOUP_CONSTEXPR20 size_t getNumBytes() const noexcept { return getNumChunks() * getBytesPerChunk(); }
		[[nodiscard]] uint8_t getByte(const size_t i) const noexcept;

		[[nodiscard]] size_t getNumNibbles() const noexcept;
		[[nodiscard]] uint8_t getNibble(size_t i) const noexcept;

		[[nodiscard]] size_t getNumBits() const noexcept;
		[[nodiscard]] bool getBit(const size_t i) const noexcept;
		void setBit(const size_t i, const bool v) SOUP_EXCAL;
		void enableBit(const size_t i) SOUP_EXCAL;
		void disableBit(const size_t i) noexcept;
		[[nodiscard]] size_t getBitLength() const noexcept;
		[[nodiscard]] size_t getLowestSetBit() const noexcept;
		[[nodiscard]] bool getBitInbounds(const size_t i) const noexcept;
		void setBitInbounds(const size_t i, const bool v) noexcept;
		void enableBitInbounds(const size_t i) noexcept;
		void disableBitInbounds(const size_t i) noexcept;
		void copyFirstBits(const Bigint& b, size_t num);

		[[nodiscard]] size_t getDChunkInbounds(size_t i) const noexcept;
		void setDChunk(size_t i, size_t v) noexcept;

#if SOUP_BIGINT_USE_INTRIN
		[[nodiscard]] static constexpr uint8_t getChunksPerQChunk() noexcept
		{
			return 128 / getBitsPerChunk();
		}

		[[nodiscard]] __m128i getQChunk(size_t i) const noexcept;
		void setQChunk(size_t i, __m128i v) noexcept;
#endif

		void reset() noexcept;
		[[nodiscard]] SOUP_CONSTEXPR20 bool isZero() const noexcept { return getNumChunks() == 0; }
		[[nodiscard]] operator bool() const noexcept;
		operator int() const noexcept = delete;

		[[nodiscard]] int cmp(const Bigint& b) const noexcept;
		[[nodiscard]] int cmpUnsigned(const Bigint& b) const noexcept;
		[[nodiscard]] bool operator == (const Bigint& b) const noexcept;
		[[nodiscard]] bool operator != (const Bigint& b) const noexcept;
		[[nodiscard]] bool operator > (const Bigint& b) const noexcept;
		[[nodiscard]] bool operator >= (const Bigint& b) const noexcept;
		[[nodiscard]] bool operator < (const Bigint& b) const noexcept;
		[[nodiscard]] bool operator <= (const Bigint& b) const noexcept;

		[[nodiscard]] bool operator == (const chunk_t v) const noexcept;
		[[nodiscard]] bool operator != (const chunk_t v) const noexcept;
		[[nodiscard]] bool operator < (const chunk_t v) const noexcept;
		[[nodiscard]] bool operator <= (const chunk_t v) const noexcept;

		void operator=(chunk_signed_t v) SOUP_EXCAL;
		void operator=(chunk_t v) SOUP_EXCAL;
		void operator=(intmax_t v) SOUP_EXCAL;
		void operator=(size_t v) SOUP_EXCAL;
	private:
		void setChunks(chunk_t v) SOUP_EXCAL;
		void setChunks(size_t v) SOUP_EXCAL;
	public:
		void operator=(Bigint&& b) noexcept;
		void operator=(const Bigint& b) SOUP_EXCAL;

		void operator+=(const Bigint& b) SOUP_EXCAL;
		void addUnsigned(const Bigint& b) SOUP_EXCAL;
		void operator-=(const Bigint& subtrahend) SOUP_EXCAL;
		void subUnsigned(const Bigint& subtrahend) noexcept;
		void operator*=(const Bigint& b) SOUP_EXCAL;
		void operator/=(const Bigint& divisor) SOUP_EXCAL;
		void operator%=(const Bigint& divisor) SOUP_EXCAL;
		[[nodiscard]] std::pair<Bigint, Bigint> divide(const Bigint& divisor) const SOUP_EXCAL; // (Quotient, Remainder)
		void divide(const Bigint& divisor, Bigint& SOUP_UNIQADDR outQuotient, Bigint& SOUP_UNIQADDR outRemainder) const SOUP_EXCAL;
		[[nodiscard]] std::pair<Bigint, Bigint> divideUnsigned(const Bigint& divisor) const SOUP_EXCAL; // (Quotient, Remainder)
		void divideUnsigned(const Bigint& divisor, Bigint& SOUP_UNIQADDR remainder) SOUP_EXCAL;
		[[nodiscard]] chunk_t divideUnsignedSmall(chunk_t divisor) noexcept;
		[[nodiscard]] Bigint mod(const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modUnsigned(const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modUnsignedPowerof2(const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modUnsignedNotpowerof2(const Bigint& divisor) const SOUP_EXCAL;
		[[nodiscard]] bool isDivisorOf(const Bigint& dividend) const SOUP_EXCAL;
		void operator<<=(const size_t b) SOUP_EXCAL;
	private:
		SOUP_FORCEINLINE void leftShiftSmall(const unsigned int b) SOUP_EXCAL;
		SOUP_FORCEINLINE void leftShiftOne() SOUP_EXCAL;
	public:
		void operator>>=(size_t b) noexcept;
		void operator|=(const Bigint& b) SOUP_EXCAL;
		void operator&=(const Bigint& b) noexcept;

		[[nodiscard]] Bigint operator+(const Bigint& b) const SOUP_EXCAL;
		Bigint& operator++() SOUP_EXCAL;
		[[nodiscard]] Bigint operator++(int) SOUP_EXCAL;
		[[nodiscard]] Bigint operator-(const Bigint& subtrahend) const SOUP_EXCAL;
		Bigint& operator--() SOUP_EXCAL;
		[[nodiscard]] Bigint operator--(int) SOUP_EXCAL;
		[[nodiscard]] Bigint operator*(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint multiplySimple(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint multiplyKaratsuba(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint multiplyKaratsubaUnsigned(const Bigint& b/*, size_t recursions = 0*/) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator/(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator%(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator<<(size_t b) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator>>(size_t b) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator|(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint operator&(const Bigint& b) const SOUP_EXCAL;

		[[nodiscard]] bool isNegative() const noexcept;
		[[nodiscard]] bool isEven() const noexcept;
		[[nodiscard]] bool isOdd() const noexcept;
		[[nodiscard]] Bigint abs() const SOUP_EXCAL;
		[[nodiscard]] Bigint pow(Bigint e) const SOUP_EXCAL;
		[[nodiscard]] Bigint powNot2(Bigint e) const SOUP_EXCAL;
		[[nodiscard]] Bigint pow2() const SOUP_EXCAL; // *this to the power of 2
		[[nodiscard]] static Bigint _2pow(size_t e) SOUP_EXCAL; // 2 to the power of e
		[[nodiscard]] size_t getTrailingZeroes(const Bigint& base) const SOUP_EXCAL;
		[[nodiscard]] size_t getTrailingZeroesBinary() const noexcept;
		[[nodiscard]] Bigint gcd(Bigint v) const SOUP_EXCAL;
		Bigint gcd(Bigint b, Bigint& x, Bigint& y) const SOUP_EXCAL;
		[[nodiscard]] bool isPrimePrecheck(bool& ret) const SOUP_EXCAL;
		[[nodiscard]] bool isPrime() const SOUP_EXCAL;
		[[nodiscard]] bool isPrimeAccurate() const SOUP_EXCAL;
		[[nodiscard]] bool isPrimeAccurateNoprecheck() const SOUP_EXCAL;
		[[nodiscard]] bool isProbablePrime(const int miller_rabin_iterations = 1) const SOUP_EXCAL;
		[[nodiscard]] bool isProbablePrimeNoprecheck(const int miller_rabin_iterations = 1) const SOUP_EXCAL;
		[[nodiscard]] bool isCoprime(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] Bigint eulersTotient() const SOUP_EXCAL;
		[[nodiscard]] Bigint reducedTotient() const SOUP_EXCAL;
		[[nodiscard]] Bigint lcm(const Bigint& b) const SOUP_EXCAL;
		[[nodiscard]] bool isPowerOf2() const SOUP_EXCAL;
		[[nodiscard]] std::pair<Bigint, Bigint> factorise() const SOUP_EXCAL;
		[[nodiscard]] Bigint sqrtCeil() const SOUP_EXCAL;
		[[nodiscard]] Bigint sqrtFloor() const SOUP_EXCAL;
		[[nodiscard]] std::pair<Bigint, Bigint> splitAt(size_t chunk) const SOUP_EXCAL;

		// Operations under a modulus
		[[nodiscard]] Bigint modMulInv(const Bigint& m) const; // *this ^ -1 mod m
		static void modMulInv2Coprimes(const Bigint& a, const Bigint& m, Bigint& x, Bigint& y) SOUP_EXCAL;
		[[nodiscard]] Bigint modMulUnsigned(const Bigint& b, const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modMulUnsignedNotpowerof2(const Bigint& b, const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modPow(const Bigint& e, const Bigint& m) const;
		[[nodiscard]] Bigint modPowMontgomery(const Bigint& e, const Bigint& m) const;
		[[nodiscard]] Bigint modPowMontgomery(const Bigint& e, size_t re, const Bigint& r, const Bigint& m, const Bigint& r_mod_mul_inv, const Bigint& m_mod_mul_inv, const Bigint& one_mont) const SOUP_EXCAL;
		[[nodiscard]] Bigint modPowBasic(const Bigint& e, const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint modDiv(const Bigint& divisor, const Bigint& m) const;

		// Montgomery operations, assuming an odd modulus
		[[nodiscard]] size_t montgomeryREFromM() const noexcept;
		[[nodiscard]] static Bigint montgomeryRFromRE(size_t re) SOUP_EXCAL;
		[[nodiscard]] Bigint montgomeryRFromM() const SOUP_EXCAL;
		[[nodiscard]] Bigint enterMontgomerySpace(const Bigint& r, const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint leaveMontgomerySpace(const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint leaveMontgomerySpaceEfficient(const Bigint& r_mod_mul_inv, const Bigint& m) const SOUP_EXCAL;
		[[nodiscard]] Bigint montgomeryMultiply(const Bigint& b, const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint montgomeryMultiplyEfficient(const Bigint& b, const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const SOUP_EXCAL;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, const Bigint& m, const Bigint& m_mod_mul_inv) const;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const SOUP_EXCAL;

		bool toPrimitive(size_t& out) const;

		template <typename Str = std::string>
		[[nodiscard]] Str toString() const
		{
			Str str{};
			Bigint quotient(*this);
			quotient.negative = false;
			if (quotient.isZero())
			{
				str.append(1, '0');
			}
			else
			{
				do
				{
					auto remainder = quotient.divideUnsignedSmall(10);
					str.insert(0, 1, (char)('0' + remainder));
				} while (!quotient.isZero());
			}
			if (negative)
			{
				str.insert(0, 1, '-');
			}
			return str;
		}

		template <typename Str = std::string>
		[[nodiscard]] Str toStringBinary(bool prefix = false) const
		{
			Str str{};
			size_t i = getNumBits();
			if (i == 0)
			{
				str.push_back('0');
			}
			else
			{
				// skip leading zeroes
				while (i-- != 0 && !getBit(i));
				str.reserve(i + 1 + (prefix * 2) + negative);
				do
				{
					str.push_back('0' + getBit(i));
				} while (i-- != 0);
			}
			if (prefix)
			{
				str.insert(0, 1, 'b');
				str.insert(0, 1, '0');
			}
			if (negative)
			{
				str.insert(0, 1, '-');
			}
			return str;
		}

		[[nodiscard]] std::string toStringHex(bool prefix = false) const;
		[[nodiscard]] std::string toStringHexLower(bool prefix = false) const;
	private:
		[[nodiscard]] std::string toStringHexImpl(bool prefix, const char* map) const;

	public:
		[[nodiscard]] static Bigint fromBinary(const std::string& msg) SOUP_EXCAL;
		[[nodiscard]] static Bigint fromBinary(const void* data, size_t size) SOUP_EXCAL;
		[[nodiscard]] std::string toBinary() const SOUP_EXCAL;
		[[nodiscard]] std::string toBinary(size_t min_bytes) const SOUP_EXCAL;

		SOUP_PACKET_IO(s) SOUP_EXCAL
		{
			SOUP_IF_ISREAD
			{
				std::string str;
				SOUP_IF_LIKELY(s.str_lp_u64_dyn(str))
				{
					*this = fromBinary(str);
					return true;
				}
				return false;
			}
			SOUP_ELSEIF_ISWRITE
			{
				return s.str_lp_u64_dyn(toBinary());
			}
		}

		SOUP_STRINGIFYABLE(Bigint)
	};

	namespace literals
	{
		inline Bigint operator ""_b(unsigned long long v)
		{
			return Bigint((size_t)v);
		}

		inline Bigint operator ""_b(const char* str, size_t len)
		{
			return Bigint::fromString(str, len);
		}
	}

	struct RandomProbablePrimeTask : public PromiseTask<Bigint>
	{
		const size_t bits;
		const int miller_rabin_iterations;

		int it;

		RandomProbablePrimeTask(size_t bits, int miller_rabin_iterations)
			: bits(bits), miller_rabin_iterations(miller_rabin_iterations), it(miller_rabin_iterations)
		{
		}

		void onTick() SOUP_EXCAL final
		{
			if (it != miller_rabin_iterations)
			{
				if (result.isProbablePrimeNoprecheck(1))
				{
					if (++it == miller_rabin_iterations)
					{
						setWorkDone();
					}
				}
				else
				{
					it = miller_rabin_iterations;
				}
			}
			else
			{
				result = Bigint::random(bits);
				result.enableBitInbounds(0);
				bool pass;
				if (!result.isPrimePrecheck(pass)
					|| pass
					)
				{
					it = 0;
				}
			}
		}

		[[nodiscard]] int getSchedulingDisposition() const noexcept final
		{
			return HIGH_FREQUENCY;
		}
	};
}
