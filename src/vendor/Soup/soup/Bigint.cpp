#include "Bigint.hpp"

#include "Bitset.hpp"
#include "bitutil.hpp"
#include "branchless.hpp"
#include "CpuInfo.hpp"
#include "Endian.hpp"
#include "Exception.hpp"
#include "ObfusString.hpp"
#include "rand.hpp"
#include "RngInterface.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	using chunk_t = Bigint::chunk_t;
	using chunk_signed_t = Bigint::chunk_signed_t;

	Bigint::Bigint(chunk_signed_t v) SOUP_EXCAL
		: Bigint()
	{
		operator =(v);
	}

	Bigint::Bigint(chunk_t v, bool negative) SOUP_EXCAL
		: negative(negative)
	{
		setChunks(v);
	}

	Bigint::Bigint(intmax_t v) SOUP_EXCAL
		: Bigint()
	{
		operator =(v);
	}

	Bigint::Bigint(size_t v, bool negative) SOUP_EXCAL
		: negative(negative)
	{
		setChunks(v);
	}

	Bigint::Bigint(Bigint&& b) noexcept
		: chunks(std::move(b.chunks)), negative(b.negative)
	{
	}

	Bigint::Bigint(const Bigint& b) SOUP_EXCAL
		: chunks(b.chunks), negative(b.negative)
	{
	}

	Bigint Bigint::fromString(const std::string& str)
	{
		return fromString(str.data(), str.size());
	}

	Bigint Bigint::fromString(const char* str, size_t len)
	{
		Bigint res{};
		if (len != 0)
		{
			const bool neg = (str[0] == '-');
			if (neg)
			{
				++str;
				--len;
			}
			if (len > 2 && str[0] == '0')
			{
				if (str[1] == 'b' || str[1] == 'B')
				{
					res.fromStringImplBinary(str + 2, len - 2);
				}
				else if (str[1] == 'x' || str[1] == 'X')
				{
					res.fromStringImplHex(str + 2, len - 2);
				}
				else
				{
					res.fromStringImplDecimal(str, len);
				}
			}
			else
			{
				res.fromStringImplDecimal(str, len);
			}
			res.negative = neg;
		}
		return res;
	}

	Bigint Bigint::fromStringHex(const char* str, size_t len)
	{
		Bigint res{};
		if (len != 0)
		{
			const bool neg = (str[0] == '-');
			if (neg)
			{
				++str;
				--len;
			}
			res.fromStringImplHex(str, len);
			res.negative = neg;
		}
		return res;
	}

	void Bigint::fromStringImplBinary(const char* str, size_t len)
	{
		for (size_t i = 0; i != len; ++i)
		{
			if (str[i] != '0')
			{
				enableBit(len - 1 - i);
			}
		}
	}

	void Bigint::fromStringImplDecimal(const char* str, size_t len)
	{
		for (size_t i = 0; i != len; ++i)
		{
			*this *= Bigint((chunk_t)10u);
			*this += (chunk_t)(str[i] - '0');
		}
	}

	void Bigint::fromStringImplHex(const char* str, size_t len)
	{
		for (size_t i = 0; i != len; ++i)
		{
			*this <<= 4u;
			if (str[i] >= 'a')
			{
				*this |= (chunk_t)(str[i] - ('a' - 10u));
			}
			else if (str[i] >= 'A')
			{
				*this |= (chunk_t)(str[i] - ('A' - 10u));
			}
			else
			{
				*this |= (chunk_t)(str[i] - '0');
			}
		}
	}

	Bigint Bigint::random(size_t bits) SOUP_EXCAL
	{
		Bigint res{};
		if ((bits % getBitsPerChunk()) == 0)
		{
			bits /= getBitsPerChunk();
			for (size_t i = 0; i != bits; ++i)
			{
				res.chunks.emplace_back(rand.t<chunk_t>(0, -1));
			}
		}
		else
		{
			for (size_t i = 0; i != bits; ++i)
			{
				if (rand.coinflip())
				{
					res.enableBit(i);
				}
			}
		}
		return res;
	}

	Bigint Bigint::random(RngInterface& rng, size_t bits) SOUP_EXCAL
	{
		Bigint res{};
		if ((bits % getBitsPerChunk()) == 0)
		{
			bits /= getBitsPerChunk();
			for (size_t i = 0; i != bits; ++i)
			{
				res.chunks.emplace_back((chunk_t)rng.generate());
			}
		}
		else
		{
			for (size_t i = 0; i != bits; ++i)
			{
				if (rng.coinflip())
				{
					res.enableBit(i);
				}
			}
		}
		return res;
	}

	Bigint Bigint::randomProbablePrime(const size_t bits, const int miller_rabin_iterations) SOUP_EXCAL
	{
		Bigint i = random(bits);
		for (; i.enableBit(0), !i.isProbablePrime(miller_rabin_iterations); i = random(bits));
		return i;
	}

	Bigint Bigint::randomProbablePrime(RngInterface& rng, const size_t bits, const int miller_rabin_iterations) SOUP_EXCAL
	{
		Bigint i = random(rng, bits);
		for (; i.enableBit(0), !i.isProbablePrime(miller_rabin_iterations); i = random(rng, bits));
		return i;
	}

	chunk_t Bigint::getChunk(size_t i) const noexcept
	{
		if (i < getNumChunks())
		{
			return getChunkInbounds(i);
		}
		return 0;
	}

	void Bigint::setChunk(size_t i, chunk_t v) SOUP_EXCAL
	{
		if (i < chunks.size())
		{
			setChunkInbounds(i, v);
		}
		else
		{
			addChunk(v);
		}
	}

	void Bigint::addChunk(size_t i, chunk_t v) SOUP_EXCAL
	{
		while (i != chunks.size())
		{
			addChunk(0);
		}
		addChunk(v);
	}

	void Bigint::addChunk(chunk_t v) SOUP_EXCAL
	{
		chunks.emplace_back(v);
	}

	void Bigint::shrink() noexcept
	{
		size_t i = chunks.size();
		for (; i != 0; --i)
		{
			if (chunks[i - 1] != 0)
			{
				break;
			}
		}
		if (i != chunks.size())
		{
#if SOUP_BIGINT_USE_INTVECTOR
			chunks.erase(i, chunks.size());
#else
			chunks.erase(chunks.cbegin() + i, chunks.cend());
#endif
		}
	}

	uint8_t Bigint::getByte(const size_t i) const noexcept
	{
		auto j = i / getBytesPerChunk();
		auto k = i % getBytesPerChunk();

		if (j < chunks.size())
		{
			return (reinterpret_cast<const uint8_t*>(&chunks[j]))[k];
		}
		return 0;
	}

	size_t Bigint::getNumNibbles() const noexcept
	{
		return getNumBytes() * 2;
	}

	uint8_t Bigint::getNibble(size_t i) const noexcept
	{
		return ((getByte(i / 2) >> ((i % 2) * 4)) & 0b1111);
	}

	size_t Bigint::getNumBits() const noexcept
	{
		return getNumChunks() * getBitsPerChunk();
	}

	bool Bigint::getBit(const size_t i) const noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		return (getChunk(chunk_i) >> j) & 1;
	}

	void Bigint::setBit(const size_t i, const bool v) SOUP_EXCAL
	{
#if true
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		if (chunk_i < chunks.size())
		{
			Bitset<chunk_t>::at(chunks[chunk_i]).set(static_cast<uint8_t>(j), v);
		}
		else if (v)
		{
			addChunk(1 << j);
		}
#else
		if (v)
		{
			enableBit(i);
		}
		else
		{
			disableBit(i);
		}
#endif
	}

	void Bigint::enableBit(const size_t i) SOUP_EXCAL
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		chunk_t mask = (static_cast<chunk_t>(1) << j);

		if (chunk_i < chunks.size())
		{
			chunks[chunk_i] |= mask;
		}
		else
		{
			addChunk(chunk_i, mask);
		}
	}

	void Bigint::disableBit(const size_t i) noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		if (chunk_i < chunks.size())
		{
			chunks[chunk_i] &= ~(1 << j);
		}
	}

	size_t Bigint::getBitLength() const noexcept
	{
		size_t len = getNumBits();
		while (len-- != 0 && !getBit(len));
		return len + 1;
	}

	size_t Bigint::getLowestSetBit() const noexcept
	{
		const auto nb = getNumBits();
		for (size_t i = 0; i != nb; ++i)
		{
			if (getBit(i))
			{
				return i;
			}
		}
		return -1;
	}

	bool Bigint::getBitInbounds(const size_t i) const noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		return (getChunkInbounds(chunk_i) >> j) & 1;
	}

	void Bigint::setBitInbounds(const size_t i, const bool v) noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		Bitset<chunk_t>::at(chunks[chunk_i]).set(static_cast<uint8_t>(j), v);
	}

	void Bigint::enableBitInbounds(const size_t i) noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		chunks[chunk_i] |= (1 << j);
	}

	void Bigint::disableBitInbounds(const size_t i) noexcept
	{
		auto chunk_i = i / getBitsPerChunk();
		auto j = i % getBitsPerChunk();

		chunks[chunk_i] &= ~(1 << j);
	}

	void Bigint::copyFirstBits(const Bigint& b, size_t num)
	{
		const size_t num_chunks = (num / getBitsPerChunk());

		size_t i = 0;
		for (; i != num_chunks; ++i)
		{
			setChunk(i, b.getChunk(i));
		}
		i *= getBitsPerChunk();
		for (; i != num; ++i)
		{
			setBit(i, b.getBit(i));
		}
	}

	size_t Bigint::getDChunkInbounds(size_t i) const noexcept
	{
		return *reinterpret_cast<const size_t*>(&chunks[i * 2]);
	}

	void Bigint::setDChunk(size_t i, size_t v) noexcept
	{
		*reinterpret_cast<size_t*>(&chunks[i * 2]) = v;
	}

#if SOUP_BIGINT_USE_INTRIN
	__m128i Bigint::getQChunk(size_t i) const noexcept
	{
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(&chunks[i * getChunksPerQChunk()]));
	}

	void Bigint::setQChunk(size_t i, __m128i v) noexcept
	{
		*reinterpret_cast<__m128i*>(&chunks[i * getChunksPerQChunk()]) = v;
	}
#endif

	void Bigint::reset() noexcept
	{
		chunks.clear();
		negative = false;
	}

	Bigint::operator bool() const noexcept
	{
		return !isZero();
	}

	int Bigint::cmp(const Bigint& b) const noexcept
	{
		if (getNumChunks() != b.getNumChunks())
		{
			return branchless::trinary(getNumChunks() > b.getNumChunks(), +1, -1);
		}
		if (negative ^ b.negative)
		{
			return branchless::trinary(negative, -1, +1);
		}
		size_t i = chunks.size();
		while (i != 0)
		{
			--i;
			if (getChunkInbounds(i) != b.getChunkInbounds(i))
			{
				return branchless::trinary(getChunkInbounds(i) > b.getChunkInbounds(i), +1, -1);
			}
		}
		return 0;
	}

	int Bigint::cmpUnsigned(const Bigint& b) const noexcept
	{
		if (getNumChunks() != b.getNumChunks())
		{
			return branchless::trinary(getNumChunks() > b.getNumChunks(), +1, -1);
		}
		size_t i = chunks.size();
		while (i != 0)
		{
			--i;
			if (getChunk(i) != b.getChunk(i))
			{
				return branchless::trinary(getChunk(i) > b.getChunk(i), +1, -1);
			}
		}
		return 0;
	}

	bool Bigint::operator==(const Bigint& b) const noexcept
	{
		return cmp(b) == 0;
	}

	bool Bigint::operator!=(const Bigint& b) const noexcept
	{
		return cmp(b) != 0;
	}

	bool Bigint::operator>(const Bigint& b) const noexcept
	{
		return cmp(b) > 0;
	}

	bool Bigint::operator>=(const Bigint& b) const noexcept
	{
		return cmp(b) >= 0;
	}

	bool Bigint::operator<(const Bigint& b) const noexcept
	{
		return cmp(b) < 0;
	}

	bool Bigint::operator<=(const Bigint& b) const noexcept
	{
		return cmp(b) <= 0;
	}

	bool Bigint::operator==(const chunk_t v) const noexcept
	{
		return !negative && getNumChunks() == 1 && getChunk(0) == v;
	}

	bool Bigint::operator!=(const chunk_t v) const noexcept
	{
		return !operator==(v);
	}

	bool Bigint::operator<(const chunk_t v) const noexcept
	{
		return negative || (getNumChunks() == 0 && v != 0) || (getNumChunks() == 1 && getChunk(0) < v);
	}

	bool Bigint::operator<=(const chunk_t v) const noexcept
	{
		return negative || getNumChunks() == 0 || (getNumChunks() == 1 && getChunk(0) <= v);
	}

	void Bigint::operator=(chunk_signed_t v) SOUP_EXCAL
	{
		negative = (v < 0);
		if (negative)
		{
			setChunks((chunk_t)(v * -1));
		}
		else
		{
			setChunks((chunk_t)v);
		}
	}

	void Bigint::operator=(chunk_t v) SOUP_EXCAL
	{
		setChunks(v);
		negative = false;
	}

	void Bigint::operator=(intmax_t v) SOUP_EXCAL
	{
		negative = (v < 0);
		if (negative)
		{
			setChunks((size_t)(v * -1));
		}
		else
		{
			setChunks((size_t)v);
		}
	}

	void Bigint::operator=(size_t v) SOUP_EXCAL
	{
		setChunks(v);
		negative = false;
	}

	void Bigint::setChunks(chunk_t v) SOUP_EXCAL
	{
		chunks.clear();
		if (v != 0)
		{
			chunks.emplace_back(v);
		}
	}

	void Bigint::setChunks(size_t v) SOUP_EXCAL
	{
		const chunk_t carry = getCarry(v);
		if (carry == 0)
		{
			setChunks((chunk_t)v);
		}
		else
		{
#if SOUP_BIGINT_USE_INTVECTOR
			chunks.clear();
			chunks.emplace_back((chunk_t)v);
			chunks.emplace_back(carry);
#else
			chunks = std::vector<chunk_t>{ (chunk_t)v, carry };
#endif
		}
	}

	void Bigint::operator=(Bigint&& b) noexcept
	{
		chunks = std::move(b.chunks);
		negative = b.negative;
	}

	void Bigint::operator=(const Bigint& b) SOUP_EXCAL
	{
		chunks = b.chunks;
		negative = b.negative;
	}

	void Bigint::operator+=(const Bigint& b) SOUP_EXCAL
	{
		if (negative ^ b.negative)
		{
			subUnsigned(b);
		}
		else
		{
			addUnsigned(b);
		}
	}

	void Bigint::addUnsigned(const Bigint& b) SOUP_EXCAL
	{
		chunk_t carry = 0;
		if (cmp(b) >= 0)
		{
			const size_t nc = getNumChunks();
			const size_t b_nc = b.getNumChunks();
			SOUP_ASSUME(nc >= b_nc);
			size_t i = 0;
			for (; i != b_nc; ++i)
			{
				const size_t x = getChunkInbounds(i);
				const size_t y = b.getChunkInbounds(i);
				size_t res = (x + y + carry);
				setChunkInbounds(i, (chunk_t)res);
				carry = getCarry(res);
			}
			for (; i != nc; ++i)
			{
				const size_t x = getChunkInbounds(i);
				const size_t y = 0;
				size_t res = (x + y + carry);
				setChunkInbounds(i, (chunk_t)res);
				carry = getCarry(res);
			}
		}
		else
		{
			const size_t j = b.getNumChunks();

			for (auto delta = j - getNumChunks(); delta--; )
			{
				addChunk(0);
			}

			for (size_t i = 0; i != j; ++i)
			{
				const size_t x = b.getChunkInbounds(i);
				const size_t y = getChunkInbounds(i);
				size_t res = (x + y + carry);
				setChunkInbounds(i, (chunk_t)res);
				carry = getCarry(res);
			}
		}
		if (carry != 0)
		{
			addChunk(carry);
		}
	}

	void Bigint::operator-=(const Bigint& subtrahend) SOUP_EXCAL
	{
		if (negative ^ subtrahend.negative)
		{
			addUnsigned(subtrahend);
		}
		else
		{
			subUnsigned(subtrahend);
		}
	}

	void Bigint::subUnsigned(const Bigint& subtrahend) noexcept
	{
		const auto cmp_res = cmp(subtrahend);
		if (cmp_res == 0)
		{
			reset();
			return;
		}
		if (cmp_res < 0)
		{
			Bigint res(subtrahend);
			res.subUnsigned(*this);
			chunks = std::move(res.chunks);
			negative ^= 1;
			return;
		}
		size_t carry = 0;
		const size_t nc = getNumChunks();
		const size_t b_nc = subtrahend.getNumChunks();
		size_t i = 0;
		for (; i != b_nc; ++i)
		{
			const size_t x = getChunkInbounds(i);
			const size_t y = subtrahend.getChunkInbounds(i);
			size_t res = (x - y - carry);
			setChunkInbounds(i, (chunk_t)res);
			carry = (getCarry(res) != 0);
		}
		for (; i != nc; ++i)
		{
			const size_t x = getChunkInbounds(i);
			const size_t y = 0;
			size_t res = (x - y - carry);
			setChunkInbounds(i, (chunk_t)res);
			carry = (getCarry(res) != 0);
		}
		shrink();
	}

	void Bigint::operator*=(const Bigint& b) SOUP_EXCAL
	{
		*this = (*this * b);
	}

	void Bigint::operator/=(const Bigint& divisor) SOUP_EXCAL
	{
		*this = divide(divisor).first;
	}

	void Bigint::operator%=(const Bigint& divisor) SOUP_EXCAL
	{
		if (*this >= divisor)
		{
			*this = mod(divisor);
		}
	}

	std::pair<Bigint, Bigint> Bigint::divide(const Bigint& divisor) const SOUP_EXCAL
	{
		std::pair<Bigint, Bigint> res;
		divide(divisor, res.first, res.second);
		return res;
	}

	void Bigint::divide(const Bigint& divisor, Bigint& SOUP_UNIQADDR outQuotient, Bigint& SOUP_UNIQADDR outRemainder) const SOUP_EXCAL
	{
		outQuotient.reset();
		outRemainder.reset();

		if (divisor.negative)
		{
			Bigint divisor_cpy(divisor);
			divisor_cpy.negative = false;
			divide(divisor_cpy, outQuotient, outRemainder);
			outQuotient.negative ^= 1;
		}
		else if (negative)
		{
			Bigint dividend(*this);
			dividend.negative = false;
			dividend.divide(divisor, outQuotient, outRemainder);
			outQuotient.negative ^= 1;
			if (!outRemainder.isZero())
			{
				outQuotient -= Bigint((chunk_t)1u);
				outRemainder = divisor - outRemainder;
			}
		}
		else
		{
			Bigint dividend(*this);
			dividend.divideUnsigned(divisor, outRemainder);
			outQuotient = std::move(dividend);
		}
	}

	std::pair<Bigint, Bigint> Bigint::divideUnsigned(const Bigint& divisor) const SOUP_EXCAL
	{
		std::pair<Bigint, Bigint> res{};
		res.first = *this;
		res.first.divideUnsigned(divisor, res.second);
		return res;
	}

	void Bigint::divideUnsigned(const Bigint& divisor, Bigint& SOUP_UNIQADDR remainder) SOUP_EXCAL
	{
		remainder.reset();
		SOUP_IF_LIKELY (!divisor.isZero())
		{
			if (divisor == (chunk_t)2u)
			{
				remainder = (chunk_t)getBit(0);
				*this >>= 1u;
			}
			else if (divisor.getNumChunks() == 1)
			{
				remainder = divideUnsignedSmall(divisor.getChunkInbounds(0));
			}
			else
			{
				for (size_t i = getNumBits(); i-- != 0; )
				{
					const auto b = getBitInbounds(i);
					remainder.leftShiftOne();
					remainder.setBit(0, b);
					if (remainder >= divisor)
					{
						remainder -= divisor;
						enableBitInbounds(i);
					}
					else
					{
						disableBitInbounds(i);
					}
				}
				shrink();
			}
		}
	}

	chunk_t Bigint::divideUnsignedSmall(chunk_t divisor) noexcept
	{
		size_t remainder = 0;
		for (size_t i = getNumBits(); i-- != 0; )
		{
			const auto b = getBitInbounds(i);
			remainder <<= 1;
			Bitset<size_t>::at(remainder).set(0, b);
			if (remainder >= divisor)
			{
				remainder -= divisor;
				enableBitInbounds(i);
			}
			else
			{
				disableBitInbounds(i);
			}
		}
		shrink();
		return (chunk_t)remainder;
	}

	Bigint Bigint::mod(const Bigint& m) const SOUP_EXCAL
	{
		if (!negative && !m.negative)
		{
			return modUnsigned(m);
		}
		return divide(m).second;
	}

	Bigint Bigint::modUnsigned(const Bigint& m) const SOUP_EXCAL
	{
		auto m_minus_1 = (m - Bigint((chunk_t)1u));
		if ((m & m_minus_1).isZero())
		{
			return (*this & m_minus_1);
		}
		return modUnsignedNotpowerof2(m);
	}

	Bigint Bigint::modUnsignedPowerof2(const Bigint& m) const SOUP_EXCAL
	{
		return (*this & (m - Bigint((chunk_t)1u)));
	}

	Bigint Bigint::modUnsignedNotpowerof2(const Bigint& divisor) const SOUP_EXCAL
	{
		Bigint remainder{};
		for (size_t i = getNumBits(); i-- != 0; )
		{
			const auto b = getBitInbounds(i);
			remainder.leftShiftOne();
			remainder.setBit(0, b);
			if (remainder >= divisor)
			{
				remainder.subUnsigned(divisor);
			}
		}
		return remainder;
	}

	bool Bigint::isDivisorOf(const Bigint& dividend) const SOUP_EXCAL
	{
		return (dividend % *this).isZero();
	}

	void Bigint::operator<<=(const size_t b) SOUP_EXCAL
	{
		if ((b % getBitsPerChunk()) == 0)
		{
#if SOUP_BIGINT_USE_INTVECTOR
			chunks.emplaceZeroesFront(b / getBitsPerChunk());
#else
			chunks.insert(chunks.begin(), b / getBitsPerChunk(), 0);
#endif
			return;
		}

		if (b <= getBitsPerChunk())
		{
			return leftShiftSmall(static_cast<unsigned int>(b));
		}

		const auto nb = getNumBits();
		if (nb != 0)
		{
			// add new chunks
			for (size_t i = (b / getBitsPerChunk()) - getNumChunks(); i-- != 0; )
			{
				addChunk(0);
			}

			// move existing bits up
			for (size_t i = nb; i-- != 0; )
			{
				setBit(i + b, getBitInbounds(i));
			}

			// zero out least significant bits
			for (size_t i = 0; i != b; ++i)
			{
				disableBitInbounds(i);
			}
		}
	}

	void Bigint::leftShiftSmall(const unsigned int b) SOUP_EXCAL
	{
		chunk_t carry = 0;
		const auto nc = getNumChunks();
		size_t i = 0;
		if constexpr (ENDIAN_NATIVE == ENDIAN_LITTLE)
		{
#if true
			if (nc >= 4)
			{
				for (; i <= nc - 4; i += 4)
				{
					chunk_t c0[2];
					chunk_t c1[2];
					chunk_t c2[2];
					chunk_t c3[2];

					c0[0] = getChunkInbounds(i + 0);
					c0[1] = 0;
					c1[0] = getChunkInbounds(i + 1);
					c1[1] = 0;
					c2[0] = getChunkInbounds(i + 2);
					c2[1] = 0;
					c3[0] = getChunkInbounds(i + 3);
					c3[1] = 0;

					*reinterpret_cast<size_t*>(&c0[0]) = (*reinterpret_cast<size_t*>(&c0[0]) << b);
					*reinterpret_cast<size_t*>(&c1[0]) = (*reinterpret_cast<size_t*>(&c1[0]) << b);
					*reinterpret_cast<size_t*>(&c2[0]) = (*reinterpret_cast<size_t*>(&c2[0]) << b);
					*reinterpret_cast<size_t*>(&c3[0]) = (*reinterpret_cast<size_t*>(&c3[0]) << b);

					c0[0] |= carry;
					c1[0] |= c0[1];
					c2[0] |= c1[1];
					c3[0] |= c2[1];

					setChunkInbounds(i + 0, c0[0]);
					setChunkInbounds(i + 1, c1[0]);
					setChunkInbounds(i + 2, c2[0]);
					setChunkInbounds(i + 3, c3[0]);

					carry = c3[1];
				}
			}
#endif
			for (; i < nc; ++i)
			{
				chunk_t c[2];
				c[0] = getChunkInbounds(i);
				c[1] = 0;
				*reinterpret_cast<size_t*>(&c[0]) = (*reinterpret_cast<size_t*>(&c[0]) << b);
				setChunkInbounds(i, c[0] | carry);
				carry = c[1];
			}
		}
		else
		{
			for (; i != nc; ++i)
			{
				size_t c = getChunkInbounds(i);
				c = ((c << b) | carry);
				setChunkInbounds(i, static_cast<chunk_t>(c));
				carry = getCarry(c);
			}
		}
		if (carry != 0)
		{
			addChunk(carry);
		}
	}

	void Bigint::leftShiftOne() SOUP_EXCAL
	{
		// Could potentially leverage _mm_slli_si128 which takes an immediate operand for the shift count.
		leftShiftSmall(1);
	}

	void Bigint::operator>>=(size_t b) noexcept
	{
		const auto chunks_to_erase = (b / getBitsPerChunk());
		const auto bits_to_erase = (b % getBitsPerChunk());

		if (chunks_to_erase != 0)
		{
			if (chunks_to_erase >= getNumChunks())
			{
				reset();
				return;
			}

#if SOUP_BIGINT_USE_INTVECTOR
			chunks.eraseFirst(chunks_to_erase);
#else
			chunks.erase(chunks.begin(), chunks.begin() + chunks_to_erase);
#endif
		}

		if (bits_to_erase != 0)
		{
			b = bits_to_erase;
			if constexpr (ENDIAN_NATIVE == ENDIAN_LITTLE)
			{
				auto i = getNumChunks();
				chunk_t carry = 0;
#if true
				for (; i >= 4; i -= 4)
				{
					chunk_t c0[2];
					chunk_t c1[2];
					chunk_t c2[2];
					chunk_t c3[2];

					c0[0] = 0;
					c0[1] = getChunkInbounds(i - 1);
					c1[0] = 0;
					c1[1] = getChunkInbounds(i - 2);
					c2[0] = 0;
					c2[1] = getChunkInbounds(i - 3);
					c3[0] = 0;
					c3[1] = getChunkInbounds(i - 4);

					*reinterpret_cast<size_t*>(&c0[0]) = (*reinterpret_cast<size_t*>(&c0[0]) >> b);
					*reinterpret_cast<size_t*>(&c1[0]) = (*reinterpret_cast<size_t*>(&c1[0]) >> b);
					*reinterpret_cast<size_t*>(&c2[0]) = (*reinterpret_cast<size_t*>(&c2[0]) >> b);
					*reinterpret_cast<size_t*>(&c3[0]) = (*reinterpret_cast<size_t*>(&c3[0]) >> b);

					c0[1] |= carry;
					c1[1] |= c0[0];
					c2[1] |= c1[0];
					c3[1] |= c2[0];

					setChunkInbounds(i - 1, c0[1]);
					setChunkInbounds(i - 2, c1[1]);
					setChunkInbounds(i - 3, c2[1]);
					setChunkInbounds(i - 4, c3[1]);

					carry = c3[0];
				}
#endif
				while (i--)
				{
					chunk_t c[2];
					c[0] = 0;
					c[1] = getChunkInbounds(i);
					*reinterpret_cast<size_t*>(&c[0]) = (*reinterpret_cast<size_t*>(&c[0]) >> b);
					setChunkInbounds(i, c[1] | carry);
					carry = c[0];
				}
			}
			else
			{
				size_t nb = getNumBits();
				if (b >= nb)
				{
					reset();
					return;
				}
				for (size_t i = 0; i != nb; ++i)
				{
					setBitInbounds(i, getBit(i + b));
				}
				for (size_t i = nb, j = 0; --i, j != b; ++j)
				{
					disableBitInbounds(i);
				}
			}
			shrink();
		}
	}

	void Bigint::operator|=(const Bigint& b) SOUP_EXCAL
	{
		const auto nc = b.getNumChunks();
		size_t i = 0;
		if (nc == getNumChunks())
		{
#if SOUP_BIGINT_USE_INTRIN
			if (CpuInfo::get().supportsSSE2())
			{
				for (; i + getChunksPerQChunk() < nc; i += getChunksPerQChunk())
				{
					size_t qi = (i / getChunksPerQChunk());
					setQChunk(qi, _mm_or_si128(getQChunk(qi), b.getQChunk(qi)));
				}
			}
#endif
			for (; i + 2 < nc; i += 2)
			{
				size_t di = (i / 2);
				setDChunk(di, getDChunkInbounds(di) | b.getDChunkInbounds(di));
			}
			for (; i != nc; ++i)
			{
				setChunkInbounds(i, getChunkInbounds(i) | b.getChunkInbounds(i));
			}
		}
		else
		{
			for (; i != nc; ++i)
			{
				setChunk(i, getChunk(i) | b.getChunkInbounds(i));
			}
		}
	}

	void Bigint::operator&=(const Bigint& b) noexcept
	{
		const auto nc = getNumChunks();
		size_t i = 0;
		if (nc == b.getNumChunks())
		{
#if SOUP_BIGINT_USE_INTRIN
			if (CpuInfo::get().supportsSSE2())
			{
				for (; i + getChunksPerQChunk() < nc; i += getChunksPerQChunk())
				{
					size_t qi = (i / getChunksPerQChunk());
					setQChunk(qi, _mm_and_si128(getQChunk(qi), b.getQChunk(qi)));
				}
			}
#endif
			for (; i + 2 < nc; i += 2)
			{
				size_t di = (i / 2);
				setDChunk(di, getDChunkInbounds(di) & b.getDChunkInbounds(di));
			}
			for (; i != nc; ++i)
			{
				setChunkInbounds(i, getChunkInbounds(i) & b.getChunkInbounds(i));
			}
		}
		else
		{
			for (; i != nc; ++i)
			{
				setChunkInbounds(i, getChunkInbounds(i) & b.getChunk(i));
			}
		}
		shrink();
	}

	Bigint Bigint::operator+(const Bigint& b) const SOUP_EXCAL
	{
		Bigint res(*this);
		res += b;
		return res;
	}

	// ++a
	Bigint& Bigint::operator++() SOUP_EXCAL
	{
		*this += Bigint((chunk_t)1u);
		return *this;
	}

	// a++
	Bigint Bigint::operator++(int) SOUP_EXCAL
	{
		Bigint pre(*this);
		++*this;
		return pre;
	}

	Bigint Bigint::operator-(const Bigint& subtrahend) const SOUP_EXCAL
	{
		Bigint res(*this);
		res -= subtrahend;
		return res;
	}

	Bigint& Bigint::operator--() SOUP_EXCAL
	{
		*this -= Bigint((chunk_t)1u);
		return *this;
	}

	Bigint Bigint::operator--(int) SOUP_EXCAL
	{
		Bigint pre(*this);
		--*this;
		return pre;
	}

	Bigint Bigint::operator*(const Bigint& b) const SOUP_EXCAL
	{
		// Karatsuba's method is supposed to be faster for 1024-bit integers, but here it seems to only be effective starting at 0x4000 bits.
		SOUP_IF_UNLIKELY (getNumBits() >= 0x4000 && b.getNumBits() >= 0x4000)
		{
			return multiplyKaratsuba(b);
		}

		return multiplySimple(b);
	}

	Bigint Bigint::multiplySimple(const Bigint& b) const SOUP_EXCAL
	{
		Bigint product{};
		if (!isZero() && !b.isZero())
		{
			const auto nc = getNumChunks();
			const auto b_nc = b.getNumChunks();
#if SOUP_BIGINT_USE_INTVECTOR
			product.chunks.preallocate();
#else
			product.chunks.reserve(nc + b_nc);
#endif
			product.negative = (negative ^ b.negative);
			for (size_t j = 0; j != b_nc; ++j)
			{
				chunk_t carry = 0;
				const size_t y = b.getChunkInbounds(j);
				for (size_t i = 0; i != nc; ++i)
				{
					const size_t x = getChunkInbounds(i);
					size_t res = product.getChunk(i + j) + (x * y) + carry;
					product.setChunk(i + j, (chunk_t)res);
					carry = getCarry(res);
				}
				if (carry != 0)
				{
					product.setChunk(j + nc, carry);
				}
			}
		}
		return product;
	}

	Bigint Bigint::multiplyKaratsuba(const Bigint& b) const SOUP_EXCAL
	{
		auto product = multiplyKaratsubaUnsigned(b);
		product.negative = (negative ^ b.negative);
		return product;
	}

	Bigint Bigint::multiplyKaratsubaUnsigned(const Bigint& b/*, size_t recursions*/) const SOUP_EXCAL
	{
		//std::cout << std::string(recursions * 2, ' ') << toString() << " * " << b.toString() << "\n";

		if (getNumBits() < 0x1000 || b.getNumBits() < 0x1000)
		{
			return multiplySimple(b);
		}

		size_t half = std::min<size_t>(getNumChunks(), b.getNumChunks()) >> 1;
		size_t half_bits = (half * getBitsPerChunk());

		auto [high1, low1] = splitAt(half);
		auto [high2, low2] = b.splitAt(half);

		Bigint p1 = high1.multiplyKaratsubaUnsigned(high2/*, recursions + 1*/);
		Bigint p2 = low1.multiplyKaratsubaUnsigned(low2/*, recursions + 1*/);
		Bigint p3 = (low1 + high1).multiplyKaratsubaUnsigned(low2 + high2/*, recursions + 1*/);

		p3.subUnsigned(p1);
		p3.subUnsigned(p2);

		p1 <<= half_bits;
		p1.addUnsigned(p3);
		p1 <<= half_bits;
		p1.addUnsigned(p2);
		return p1;
	}

	Bigint Bigint::operator/(const Bigint& b) const SOUP_EXCAL
	{
		return divide(b).first;
	}

	Bigint Bigint::operator%(const Bigint& b) const SOUP_EXCAL
	{
		return divide(b).second;
	}

	Bigint Bigint::operator<<(size_t b) const SOUP_EXCAL
	{
		Bigint res(*this);
		res <<= b;
		return res;
	}

	Bigint Bigint::operator>>(size_t b) const SOUP_EXCAL
	{
		Bigint res(*this);
		res >>= b;
		return res;
	}

	Bigint Bigint::operator|(const Bigint& b) const SOUP_EXCAL
	{
		Bigint res(*this);
		res |= b;
		return res;
	}

	Bigint Bigint::operator&(const Bigint& b) const SOUP_EXCAL
	{
		Bigint res(*this);
		res &= b;
		return res;
	}

	bool Bigint::isNegative() const noexcept
	{
		return negative;
	}
	
	bool Bigint::isEven() const noexcept
	{
		return !isOdd();
	}

	bool Bigint::isOdd() const noexcept
	{
		return getBit(0);
	}

	Bigint Bigint::abs() const SOUP_EXCAL
	{
		Bigint res(*this);
		res.negative = false;
		return res;
	}

	Bigint Bigint::pow(Bigint e) const SOUP_EXCAL
	{
		if (*this == (chunk_t)2u)
		{
			size_t e_primitive;
			if (e.toPrimitive(e_primitive))
			{
				return _2pow(e_primitive);
			}
		}
		return powNot2(e);
	}

	Bigint Bigint::powNot2(Bigint e) const SOUP_EXCAL
	{
		Bigint res = Bigint((chunk_t)1u);
		Bigint base(*this);
		while (true)
		{
			if (e.getBit(0))
			{
				res *= base;
			}
			e >>= 1u;
			if (e.isZero())
			{
				break;
			}
			base *= base;
		}
		return res;
	}

	Bigint Bigint::pow2() const SOUP_EXCAL
	{
		return *this * *this;
	}

	Bigint Bigint::_2pow(size_t e) SOUP_EXCAL
	{
		return Bigint((chunk_t)1u) << e;
	}

	size_t Bigint::getTrailingZeroes(const Bigint& base) const SOUP_EXCAL
	{
		if (base == (chunk_t)2u)
		{
			return getTrailingZeroesBinary();
		}
		size_t res = 0;
		Bigint tmp(*this);
		while (!tmp.isZero())
		{
			auto pair = tmp.divide(base);
			if (!pair.second.isZero())
			{
				break;
			}
			++res;
			tmp = std::move(pair.first);
		}
		return res;
	}

	size_t Bigint::getTrailingZeroesBinary() const noexcept
	{
		const auto nc = getNumChunks();
		for (size_t i = 0; i != nc; ++i)
		{
			auto c = getChunkInbounds(i);
			if (c != 0)
			{
				return i * getBitsPerChunk() + bitutil::getLeastSignificantSetBit(getChunkInbounds(i));
			}
		}
		return getNumBits();
	}

	Bigint Bigint::gcd(Bigint v) const SOUP_EXCAL
	{
		Bigint u(*this);

		auto i = u.getTrailingZeroesBinary(); u >>= i;
		auto j = v.getTrailingZeroesBinary(); v >>= j;
		auto k = branchless::min(i, j);

		while (true)
		{
			if (u > v)
			{
				std::swap(u, v);
			}

			v -= u;

			if (v.isZero())
			{
				u <<= k;
				return u;
			}

			v >>= v.getTrailingZeroesBinary();
		}
	}

	Bigint Bigint::gcd(Bigint b, Bigint& x, Bigint& y) const SOUP_EXCAL
	{
		x.reset();
		y = Bigint((chunk_t)1u);

		Bigint u = Bigint((chunk_t)1u), v, m, n, q, r;
		Bigint a = *this;

		while (!a.isZero())
		{
			b.divide(a, q, r);
			m = x - u * q;
			n = y - v * q;

			b = std::move(a);
			a = std::move(r);
			x = std::move(u);
			y = std::move(v);
			u = std::move(m);
			v = std::move(n);
		}

		return b;
	}

	bool Bigint::isPrimePrecheck(bool& ret) const SOUP_EXCAL
	{
		if (isZero() || *this == (chunk_t)1u)
		{
			ret = false;
			return true;
		}
		if (*this <= Bigint((chunk_t)3u))
		{
			ret = true;
			return true;
		}
		if (getBit(0) == 0)
		{
			ret = false;
			return true;
		}

		if (this->modUnsigned(Bigint((chunk_t)3u)).isZero())
		{
			ret = false;
			return true;
		}
		return false;
	}

	bool Bigint::isPrime() const SOUP_EXCAL
	{
		bool preret;
		if (isPrimePrecheck(preret))
		{
			return preret;
		}

		if (getNumBits() < 128)
		{
			return isPrimeAccurateNoprecheck();
		}

		return isProbablePrimeNoprecheck(10);
	}

	bool Bigint::isPrimeAccurate() const SOUP_EXCAL
	{
		bool preret;
		if (isPrimePrecheck(preret))
		{
			return preret;
		}

		return isPrimeAccurateNoprecheck();
	}

	bool Bigint::isPrimeAccurateNoprecheck() const SOUP_EXCAL
	{
		for (Bigint i = Bigint((chunk_t)5u); i * i <= *this; i += Bigint((chunk_t)6u))
		{
			if ((*this % i).isZero() || (*this % (i + Bigint((chunk_t)2u))).isZero())
			{
				return false;
			}
		}
		return true;
	}

	bool Bigint::isProbablePrime(const int miller_rabin_iterations) const SOUP_EXCAL
	{
		bool preret;
		if (isPrimePrecheck(preret))
		{
			return preret;
		}

		return isProbablePrimeNoprecheck(miller_rabin_iterations);
	}

	bool Bigint::isProbablePrimeNoprecheck(const int miller_rabin_iterations) const SOUP_EXCAL
	{
		auto thisMinusOne = (*this - Bigint((chunk_t)1u));
		auto a = thisMinusOne.getLowestSetBit();
		auto m = (thisMinusOne >> a);

		const auto bl = getBitLength();
		for (int i = 0; i != miller_rabin_iterations; i++)
		{
			Bigint b;
			do
			{
				b = random(bl);
			} while (b >= *this || b <= Bigint((chunk_t)1u));

			int j = 0;
			Bigint z = b.modPow(m, *this);
			while (!((j == 0 && z == (chunk_t)1u) || z == thisMinusOne))
			{
				if ((j > 0 && z == (chunk_t)1u) || ++j == a)
				{
					return false;
				}
				// z = z.modPow(2u, *this);
				z = z.modMulUnsignedNotpowerof2(z, *this);
			}
		}
		return true;
	}

	bool Bigint::isCoprime(const Bigint& b) const SOUP_EXCAL
	{
		return gcd(b) == (chunk_t)1u;
	}

	Bigint Bigint::eulersTotient() const SOUP_EXCAL
	{
		Bigint res = Bigint((chunk_t)1u);
		for (Bigint i = Bigint((chunk_t)2u); i != *this; ++i)
		{
			if (isCoprime(i))
			{
				++res;
			}
		}
		return res;
	}

	Bigint Bigint::reducedTotient() const SOUP_EXCAL
	{
		if (*this <= Bigint((chunk_t)2u))
		{
			return Bigint((chunk_t)1u);
		}
		std::vector<Bigint> coprimes{};
		for (Bigint a = Bigint((chunk_t)2u); a != *this; ++a)
		{
			if (isCoprime(a))
			{
				coprimes.emplace_back(a);
			}
		}
		Bigint k = Bigint((chunk_t)2u);
		for (auto timer = coprimes.size(); timer != 0; )
		{
			for (auto i = coprimes.begin(); i != coprimes.end(); ++i)
			{
				if (i->modPow(k, *this) == (chunk_t)1u)
				{
					if (timer == 0)
					{
						break;
					}
					--timer;
				}
				else
				{
					timer = coprimes.size();
					++k;
				}
			}
		}
		return k;
	}

	Bigint Bigint::lcm(const Bigint& b) const SOUP_EXCAL
	{
		if (isZero() || b.isZero())
		{
			return Bigint();
		}
		auto a_mag = abs();
		auto b_mag = b.abs();
		return ((a_mag * b_mag) / a_mag.gcd(b_mag));
	}

	bool Bigint::isPowerOf2() const SOUP_EXCAL
	{
		return (*this & (*this - Bigint((chunk_t)1u))).isZero();
	}

	// Fermat's method
	std::pair<Bigint, Bigint> Bigint::factorise() const SOUP_EXCAL
	{
		{
			auto [q, r] = divide((chunk_t)2u);
			if (r.isZero())
			{
				return { q, (chunk_t)2u };
			}
		}

		Bigint a = sqrtCeil();

		if ((a * a) == *this)
		{
			return { a, a };
		}

		Bigint b;
		const Bigint one = (chunk_t)1u;
		while (true)
		{
			Bigint b1 = (a * a) - *this;
			b = b1.sqrtFloor();
			if ((b * b) == b1)
			{
				break;
			}
			a += one;
		}
		return { a - b, a + b };
	}

	Bigint Bigint::sqrtCeil() const SOUP_EXCAL
	{
		Bigint y = sqrtFloor();

		if (*this == (y * y))
		{
			return y;
		}

		return (y + Bigint((chunk_t)1u));
	}

	Bigint Bigint::sqrtFloor() const SOUP_EXCAL
	{
		if (isZero() || *this == (chunk_t)1u)
		{
			return *this;
		}

		const Bigint two = (chunk_t)2u;
		Bigint y = (*this / two);
		Bigint x_over_y, remainder;

		while (this->divide(y, x_over_y, remainder), y > x_over_y)
		{
			y = (x_over_y + y) / two;
		}

		return y;
	}

	std::pair<Bigint, Bigint> Bigint::splitAt(size_t chunk) const SOUP_EXCAL
	{
		Bigint hi, lo;

		size_t i = 0;
		for (; i != chunk; ++i)
		{
			lo.setChunk(i, getChunkInbounds(i));
		}

		for (size_t j = 0; i != getNumChunks(); ++i, ++j)
		{
			hi.setChunk(j, getChunkInbounds(i));
		}

		return { std::move(hi), std::move(lo) };
	}

	Bigint Bigint::modMulInv(const Bigint& m) const
	{
		Bigint x, y;
		SOUP_IF_UNLIKELY (gcd(m, x, y) != (chunk_t)1u)
		{
			SOUP_THROW(Exception(ObfusString("Modular multiplicative inverse does not exist as the numbers are not coprime").str()));
		}
		return (x % m + m) % m;
	}

	void Bigint::modMulInv2Coprimes(const Bigint& a, const Bigint& m, Bigint& x, Bigint& y) SOUP_EXCAL
	{
		a.gcd(m, x, y);
		x = ((x % m + m) % m);
		y = ((y % a + a) % a);
	}

	Bigint Bigint::modMulUnsigned(const Bigint& b, const Bigint& m) const SOUP_EXCAL
	{
		return (*this * b).modUnsigned(m);
	}

	Bigint Bigint::modMulUnsignedNotpowerof2(const Bigint& b, const Bigint& m) const SOUP_EXCAL
	{
		return (*this * b).modUnsignedNotpowerof2(m);
	}

	Bigint Bigint::modPow(const Bigint& e, const Bigint& m) const
	{
		if (m.isOdd()
			&& e.getNumBits() > 32 // arbitrary choice
			)
		{
			return modPowMontgomery(e, m);
		}
		return modPowBasic(e, m);
	}

	Bigint Bigint::modPowMontgomery(const Bigint& e, const Bigint& m) const
	{
		auto re = m.montgomeryREFromM();
		auto r = montgomeryRFromRE(re);
		Bigint m_mod_mul_inv, r_mod_mul_inv;
		modMulInv2Coprimes(m, r, m_mod_mul_inv, r_mod_mul_inv);
		return modPowMontgomery(e, re, r, m, r_mod_mul_inv, m_mod_mul_inv, r.modUnsignedNotpowerof2(m));
	}

	Bigint Bigint::modPowMontgomery(const Bigint& e, size_t re, const Bigint& r, const Bigint& m, const Bigint& r_mod_mul_inv, const Bigint& m_mod_mul_inv, const Bigint& one_mont) const SOUP_EXCAL
	{
		Bigint res = one_mont;
		Bigint base(*this);
		if (base >= m)
		{
			base = base.modUnsigned(m);
		}
		base = base.enterMontgomerySpace(r, m);
		const auto bl = e.getBitLength();
		for (size_t i = 0; i != bl; ++i)
		{
			if (e.getBitInbounds(i))
			{
				res = res.montgomeryMultiplyEfficient(base, r, re, m, m_mod_mul_inv);
			}
			base = base.montgomeryMultiplyEfficient(base, r, re, m, m_mod_mul_inv);
		}
		return res.leaveMontgomerySpaceEfficient(r_mod_mul_inv, m);
	}

	Bigint Bigint::modPowBasic(const Bigint& e, const Bigint& m) const SOUP_EXCAL
	{
		Bigint base(*this);
		if (base >= m)
		{
			base = base.modUnsigned(m);
		}
		Bigint res = Bigint((chunk_t)1u);
		const auto bl = e.getBitLength();
		for (size_t i = 0; i != bl; ++i) // while (e)
		{
			if (e.getBitInbounds(i)) // if (e & 1)
			{
				res = res.modMulUnsignedNotpowerof2(base, m);
			}
			base = base.modMulUnsignedNotpowerof2(base, m);
			// e >>= 1
		}
		return res;
	}

	Bigint Bigint::modDiv(const Bigint& divisor, const Bigint& m) const
	{
		return (*this * divisor.modMulInv(m)) % m;
	}

	// We need a positive integer r such that r >= m && r.isCoprime(m)
	// We assume an odd modulus, so any power of 2 will be coprime to it.

	size_t Bigint::montgomeryREFromM() const noexcept
	{
		return getBitLength();
	}

	Bigint Bigint::montgomeryRFromRE(size_t re) SOUP_EXCAL
	{
		return _2pow(re);
	}

	Bigint Bigint::montgomeryRFromM() const SOUP_EXCAL
	{
		return montgomeryRFromRE(montgomeryREFromM());
	}

	Bigint Bigint::enterMontgomerySpace(const Bigint& r, const Bigint& m) const SOUP_EXCAL
	{
		return modMulUnsignedNotpowerof2(r, m);
	}

	Bigint Bigint::leaveMontgomerySpace(const Bigint& r, const Bigint& m) const
	{
		return leaveMontgomerySpaceEfficient(r.modMulInv(m), m);
	}

	Bigint Bigint::leaveMontgomerySpaceEfficient(const Bigint& r_mod_mul_inv, const Bigint& m) const SOUP_EXCAL
	{
		return modMulUnsignedNotpowerof2(r_mod_mul_inv, m);
	}

	Bigint Bigint::montgomeryMultiply(const Bigint& b, const Bigint& r, const Bigint& m) const
	{
		return (*this * b).montgomeryReduce(r, m);
	}

	Bigint Bigint::montgomeryMultiplyEfficient(const Bigint& b, const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const SOUP_EXCAL
	{
		return (*this * b).montgomeryReduce(r, re, m, m_mod_mul_inv);
	}

	Bigint Bigint::montgomeryReduce(const Bigint& r, const Bigint& m) const
	{
		return montgomeryReduce(r, m, m.modMulInv(r));
	}

	Bigint Bigint::montgomeryReduce(const Bigint& r, const Bigint& m, const Bigint& m_mod_mul_inv) const
	{
		const auto r_mask = r - Bigint((chunk_t)1u);
		auto q = ((*this & r_mask) * m_mod_mul_inv);
		q &= r_mask;
		auto a = (*this - q * m) / r;
		if (a.negative)
		{
			a += m;
		}
		return a;
	}

	Bigint Bigint::montgomeryReduce(const Bigint& r, size_t re, const Bigint& m, const Bigint& m_mod_mul_inv) const SOUP_EXCAL
	{
		const auto r_mask = r - Bigint((chunk_t)1u);
		auto q = ((*this & r_mask) * m_mod_mul_inv);
		q &= r_mask;
		auto a = (*this - q * m);
		a >>= re;
		if (a.negative)
		{
			a += m;
		}
		return a;
	}

	bool Bigint::toPrimitive(size_t& out) const
	{
		switch (getNumChunks())
		{
		case 0:
			out = 0;
			return true;

		case 1:
			out = getChunk(0);
			return true;

		case 2:
			*(chunk_t*)&out = getChunk(0);
			*((chunk_t*)&out + 1) = getChunk(1);
			return true;
		}
		return false;
	}

	std::string Bigint::toStringHex(bool prefix) const
	{
		return toStringHexImpl(prefix, string::charset_hex);
	}

	std::string Bigint::toStringHexLower(bool prefix) const
	{
		return toStringHexImpl(prefix, string::charset_hex_lower);
	}

	std::string Bigint::toStringHexImpl(bool prefix, const char* map) const
	{
		std::string str{};
		size_t i = getNumNibbles();
		if (i == 0)
		{
			str.push_back('0');
		}
		else
		{
			// skip leading zeroes
			while (i-- != 0 && getNibble(i) == 0);
			str.reserve(i + 1 + (prefix * 2) + negative);
			do
			{
				str.push_back(map[getNibble(i)]);
			} while (i-- != 0);
		}
		if (prefix)
		{
			str.insert(0, 1, 'x');
			str.insert(0, 1, '0');
		}
		if (negative)
		{
			str.insert(0, 1, '-');
		}
		return str;
	}

	Bigint Bigint::fromBinary(const std::string& msg) SOUP_EXCAL
	{
		return Bigint::fromBinary(msg.data(), msg.size());
	}

	Bigint Bigint::fromBinary(const void* data, size_t size) SOUP_EXCAL
	{
		Bigint res{};
		for (size_t i = 0; i != size; ++i)
		{
			res <<= 8u;
			res |= static_cast<chunk_t>(reinterpret_cast<const uint8_t*>(data)[i]);
		}
		return res;
	}

	std::string Bigint::toBinary() const SOUP_EXCAL
	{
		std::string str{};
		size_t i = getNumBytes();
		if (i != 0)
		{
			// skip leading zeroes
			while (i-- != 0 && getByte(i) == 0);
			str.reserve(i);
			do
			{
				str.push_back(getByte(i));
			} while (i-- != 0);
		}
		return str;
	}

	std::string Bigint::toBinary(size_t bytes) const
	{
		auto bin = toBinary();
		SOUP_ASSERT(bytes >= bin.size());
		if (size_t pad = (bytes - bin.size()))
		{
			bin.insert(bin.begin(), pad, '\0');
		}
		return bin;
	}
}
