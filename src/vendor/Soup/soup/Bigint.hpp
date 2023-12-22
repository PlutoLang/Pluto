#pragma once

#include "base.hpp"

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
#include "stringifyable.hpp"
#include "Task.hpp"

namespace soup
{
	class Bigint
	{
	public:
		using chunk_t = halfsize_t;
		using chunk_signed_t = halfintmax_t;

	private:
#if SOUP_BIGINT_USE_INTVECTOR
		IntVector<chunk_t> chunks{};
#else
		std::vector<chunk_t> chunks{};
#endif
		bool negative = false;

	public:
		Bigint() noexcept = default;
		Bigint(chunk_signed_t v);
		Bigint(chunk_t v, bool negative = false);
		Bigint(intmax_t v);
		Bigint(size_t v, bool negative = false);
		Bigint(Bigint&& b);
		Bigint(const Bigint& b);

		[[nodiscard]] static Bigint fromString(const std::string& str);
		[[nodiscard]] static Bigint fromString(const char* str, size_t len);
		[[nodiscard]] static Bigint fromStringHex(const char* str, size_t len);
	private:
		void fromStringImplBinary(const char* str, size_t len);
		void fromStringImplDecimal(const char* str, size_t len);
		void fromStringImplHex(const char* str, size_t len);

	public:
		[[nodiscard]] static Bigint random(size_t bits);
		[[nodiscard]] static Bigint random(RngInterface& rng, size_t bits);
		[[nodiscard]] static Bigint randomProbablePrime(const size_t bits, const int miller_rabin_iterations = 1);
		[[nodiscard]] static Bigint randomProbablePrime(RngInterface& rng, const size_t bits, const int miller_rabin_iterations = 1);

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
		void setChunk(size_t i, chunk_t v);
		SOUP_FORCEINLINE void setChunkInbounds(size_t i, chunk_t v) noexcept { chunks[i] = v; }
		void addChunk(size_t i, chunk_t v);
		void addChunk(chunk_t v);
		void shrink() noexcept;

		[[nodiscard]] SOUP_CONSTEXPR20 size_t getNumBytes() const noexcept { return getNumChunks() * getBytesPerChunk(); }
		[[nodiscard]] uint8_t getByte(const size_t i) const noexcept;

		[[nodiscard]] size_t getNumNibbles() const noexcept;
		[[nodiscard]] uint8_t getNibble(size_t i) const noexcept;

		[[nodiscard]] size_t getNumBits() const noexcept;
		[[nodiscard]] bool getBit(const size_t i) const noexcept;
		void setBit(const size_t i, const bool v);
		void enableBit(const size_t i);
		void disableBit(const size_t i);
		[[nodiscard]] size_t getBitLength() const noexcept;
		[[nodiscard]] size_t getLowestSetBit() const noexcept;
		[[nodiscard]] bool getBitInbounds(const size_t i) const noexcept;
		void setBitInbounds(const size_t i, const bool v);
		void enableBitInbounds(const size_t i);
		void disableBitInbounds(const size_t i);
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

		void operator=(chunk_signed_t v);
		void operator=(chunk_t v);
		void operator=(intmax_t v);
		void operator=(size_t v);
	private:
		void setChunks(chunk_t v);
		void setChunks(size_t v);
	public:
		void operator=(Bigint&& b);
		void operator=(const Bigint& b);

		void operator+=(const Bigint& b);
		void addUnsigned(const Bigint& b);
		void operator-=(const Bigint& subtrahend);
		void subUnsigned(const Bigint& subtrahend);
		void operator*=(const Bigint& b);
		void operator/=(const Bigint& divisor);
		void operator%=(const Bigint& divisor);
		[[nodiscard]] std::pair<Bigint, Bigint> divide(const Bigint& divisor) const; // (Quotient, Remainder)
		[[nodiscard]] std::pair<Bigint, Bigint> divideUnsigned(const Bigint& divisor) const; // (Quotient, Remainder)
		void divideUnsigned(const Bigint& divisor, Bigint& remainder);
		[[nodiscard]] chunk_t divideUnsignedSmall(chunk_t divisor);
		[[nodiscard]] Bigint mod(const Bigint& m) const;
		[[nodiscard]] Bigint modUnsigned(const Bigint& m) const;
		[[nodiscard]] Bigint modUnsignedPowerof2(const Bigint& m) const;
		[[nodiscard]] Bigint modUnsignedNotpowerof2(const Bigint& m) const;
		[[nodiscard]] bool isDivisorOf(const Bigint& dividend) const;
		void operator<<=(const size_t b);
	private:
		void leftShiftSmall(const size_t b);
		void leftShiftOne();
	public:
		void operator>>=(const size_t b);
	private:
		void rightShiftSmall(const size_t b);
	public:
		void operator|=(const Bigint& b);
		void operator&=(const Bigint& b);

		[[nodiscard]] Bigint operator+(const Bigint& b) const;
		Bigint& operator++();
		[[nodiscard]] Bigint operator++(int);
		[[nodiscard]] Bigint operator-(const Bigint& subtrahend) const;
		Bigint& operator--();
		[[nodiscard]] Bigint operator--(int);
		[[nodiscard]] Bigint operator*(const Bigint& b) const;
		[[nodiscard]] Bigint multiplySimple(const Bigint& b) const;
		[[nodiscard]] Bigint multiplyKaratsuba(const Bigint& b) const;
		[[nodiscard]] Bigint multiplyKaratsubaUnsigned(const Bigint& b/*, size_t recursions = 0*/) const;
		[[nodiscard]] Bigint operator/(const Bigint& b) const;
		[[nodiscard]] Bigint operator%(const Bigint& b) const;
		[[nodiscard]] Bigint operator<<(size_t b) const;
		[[nodiscard]] Bigint operator>>(size_t b) const;
		[[nodiscard]] Bigint operator|(const Bigint& b) const;
		[[nodiscard]] Bigint operator&(const Bigint& b) const;

		[[nodiscard]] bool isNegative() const noexcept;
		[[nodiscard]] bool isEven() const noexcept;
		[[nodiscard]] bool isOdd() const noexcept;
		[[nodiscard]] Bigint abs() const;
		[[nodiscard]] Bigint pow(Bigint e) const;
		[[nodiscard]] Bigint powNot2(Bigint e) const;
		[[nodiscard]] Bigint pow2() const; // *this to the power of 2
		[[nodiscard]] static Bigint _2pow(size_t e); // 2 to the power of e
		[[nodiscard]] size_t getTrailingZeroes(const Bigint& base) const;
		[[nodiscard]] size_t getTrailingZeroesBinary() const;
		[[nodiscard]] Bigint gcd(Bigint v) const;
		Bigint gcd(Bigint b, Bigint& x, Bigint& y) const;
		[[nodiscard]] bool isPrimePrecheck(bool& ret) const;
		[[nodiscard]] bool isPrime() const;
		[[nodiscard]] bool isPrimeAccurate() const;
		[[nodiscard]] bool isPrimeAccurateNoprecheck() const;
		[[nodiscard]] bool isProbablePrime(const int miller_rabin_iterations = 1) const;
		[[nodiscard]] bool isProbablePrimeNoprecheck(const int miller_rabin_iterations = 1) const;
		[[nodiscard]] bool isCoprime(const Bigint& b) const;
		[[nodiscard]] Bigint eulersTotient() const;
		[[nodiscard]] Bigint reducedTotient() const;
		[[nodiscard]] Bigint lcm(const Bigint& b) const;
		[[nodiscard]] bool isPowerOf2() const;
		[[nodiscard]] std::pair<Bigint, Bigint> factorise() const;
		[[nodiscard]] Bigint sqrtCeil() const;
		[[nodiscard]] Bigint sqrtFloor() const;
		[[nodiscard]] std::pair<Bigint, Bigint> splitAt(size_t chunk) const;

		// Operations under a modulus
		[[nodiscard]] Bigint modMulInv(const Bigint& m) const; // *this ^ -1 mod m
		static void modMulInv2Coprimes(const Bigint& a, const Bigint& m, Bigint& x, Bigint& y);
		[[nodiscard]] Bigint modMulUnsigned(const Bigint& b, const Bigint& m) const;
		[[nodiscard]] Bigint modMulUnsignedNotpowerof2(const Bigint& b, const Bigint& m) const;
		[[nodiscard]] Bigint modPow(const Bigint& e, const Bigint& m) const;
		[[nodiscard]] Bigint modPowMontgomery(const Bigint& e, const Bigint& m) const;
		[[nodiscard]] Bigint modPowMontgomery(const Bigint& e, size_t re, const Bigint& r, const Bigint& m, const Bigint& r_mod_mul_inv, const Bigint& m_mod_mul_inv, const Bigint& one_mont) const;
		[[nodiscard]] Bigint modPowBasic(const Bigint& e, const Bigint& m) const;
		[[nodiscard]] Bigint modDiv(const Bigint& divisor, const Bigint& m) const;

		// Montgomery operations, assuming an odd modulus
		[[nodiscard]] size_t montgomeryREFromM() const;
		[[nodiscard]] static Bigint montgomeryRFromRE(size_t re);
		[[nodiscard]] Bigint montgomeryRFromM() const;
		[[nodiscard]] Bigint enterMontgomerySpace(const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint leaveMontgomerySpace(const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint leaveMontgomerySpaceEfficient(const Bigint& r_mod_mul_inv, const Bigint& m) const;
		[[nodiscard]] Bigint montgomeryMultiply(const Bigint& b, const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint montgomeryMultiplyEfficient(const Bigint& b, const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, const Bigint& m) const;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, const Bigint& m, const Bigint& m_mod_mul_inv) const;
		[[nodiscard]] Bigint montgomeryReduce(const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const;

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
				const Bigint TEN = (chunk_t)10u;
				Bigint remainder;
				do
				{
					quotient.divideUnsigned(TEN, remainder);
					str.insert(0, 1, (char)('0' + remainder.getChunk(0)));
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
		[[nodiscard]] static Bigint fromBinary(const std::string& msg);
		[[nodiscard]] std::string toBinary() const;
		[[nodiscard]] std::string toBinary(size_t bytes) const;

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

		void onTick() final
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

		[[nodiscard]] int getSchedulingDisposition() const final
		{
			return HIGH_FRQUENCY;
		}
	};
}
