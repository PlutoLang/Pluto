#include "md5.hpp"

// "Derived from the RSA Data Security, Inc. MD5 Message Digest Algorithm"

NAMESPACE_SOUP
{
	std::string md5::hash(const void* data, size_t len) SOUP_EXCAL
	{
		State state;
		state.append(data, len);
		state.finalise();
		return state.getDigest();
	}

	md5::State::State()
	{
		state[0] = (uint32_t)0x67452301;
		state[1] = (uint32_t)0xefcdab89;
		state[2] = (uint32_t)0x98badcfe;
		state[3] = (uint32_t)0x10325476;
		n_bytes = 0;
	}

	void md5::State::append(const void* data, size_t size) noexcept
	{
		if (!size)
		{
			return;
		}

		auto left = n_bytes % BLOCK_BYTES;
		auto fill = BLOCK_BYTES - left;

		n_bytes += size;

		if (left && size >= fill)
		{
			memcpy(buffer + left, data, fill);
			transform();
			data = (uint8_t*)data + fill;
			size -= fill;
			left = 0;
		}

		while (size >= BLOCK_BYTES)
		{
			memcpy(buffer, data, BLOCK_BYTES);
			transform();
			data = (uint8_t*)data + BLOCK_BYTES;
			size -= BLOCK_BYTES;
		}

		if (size)
		{
			memcpy(buffer + left, data, size);
		}
	}

	#define GET_ULONG_LE(n,b,i)                             \
	{                                                       \
		(n) = ( (unsigned long) (b)[(i)    ]       )        \
			| ( (unsigned long) (b)[(i) + 1] <<  8 )        \
			| ( (unsigned long) (b)[(i) + 2] << 16 )        \
			| ( (unsigned long) (b)[(i) + 3] << 24 );       \
	}

	void md5::State::transform() noexcept
	{
		unsigned long X[16], A, B, C, D;

		GET_ULONG_LE(X[0], buffer, 0);
		GET_ULONG_LE(X[1], buffer, 4);
		GET_ULONG_LE(X[2], buffer, 8);
		GET_ULONG_LE(X[3], buffer, 12);
		GET_ULONG_LE(X[4], buffer, 16);
		GET_ULONG_LE(X[5], buffer, 20);
		GET_ULONG_LE(X[6], buffer, 24);
		GET_ULONG_LE(X[7], buffer, 28);
		GET_ULONG_LE(X[8], buffer, 32);
		GET_ULONG_LE(X[9], buffer, 36);
		GET_ULONG_LE(X[10], buffer, 40);
		GET_ULONG_LE(X[11], buffer, 44);
		GET_ULONG_LE(X[12], buffer, 48);
		GET_ULONG_LE(X[13], buffer, 52);
		GET_ULONG_LE(X[14], buffer, 56);
		GET_ULONG_LE(X[15], buffer, 60);

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

		A = this->state[0];
		B = this->state[1];
		C = this->state[2];
		D = this->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

		P(A, B, C, D, 0, 7, 0xD76AA478);
		P(D, A, B, C, 1, 12, 0xE8C7B756);
		P(C, D, A, B, 2, 17, 0x242070DB);
		P(B, C, D, A, 3, 22, 0xC1BDCEEE);
		P(A, B, C, D, 4, 7, 0xF57C0FAF);
		P(D, A, B, C, 5, 12, 0x4787C62A);
		P(C, D, A, B, 6, 17, 0xA8304613);
		P(B, C, D, A, 7, 22, 0xFD469501);
		P(A, B, C, D, 8, 7, 0x698098D8);
		P(D, A, B, C, 9, 12, 0x8B44F7AF);
		P(C, D, A, B, 10, 17, 0xFFFF5BB1);
		P(B, C, D, A, 11, 22, 0x895CD7BE);
		P(A, B, C, D, 12, 7, 0x6B901122);
		P(D, A, B, C, 13, 12, 0xFD987193);
		P(C, D, A, B, 14, 17, 0xA679438E);
		P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

		P(A, B, C, D, 1, 5, 0xF61E2562);
		P(D, A, B, C, 6, 9, 0xC040B340);
		P(C, D, A, B, 11, 14, 0x265E5A51);
		P(B, C, D, A, 0, 20, 0xE9B6C7AA);
		P(A, B, C, D, 5, 5, 0xD62F105D);
		P(D, A, B, C, 10, 9, 0x02441453);
		P(C, D, A, B, 15, 14, 0xD8A1E681);
		P(B, C, D, A, 4, 20, 0xE7D3FBC8);
		P(A, B, C, D, 9, 5, 0x21E1CDE6);
		P(D, A, B, C, 14, 9, 0xC33707D6);
		P(C, D, A, B, 3, 14, 0xF4D50D87);
		P(B, C, D, A, 8, 20, 0x455A14ED);
		P(A, B, C, D, 13, 5, 0xA9E3E905);
		P(D, A, B, C, 2, 9, 0xFCEFA3F8);
		P(C, D, A, B, 7, 14, 0x676F02D9);
		P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x,y,z) (x ^ y ^ z)

		P(A, B, C, D, 5, 4, 0xFFFA3942);
		P(D, A, B, C, 8, 11, 0x8771F681);
		P(C, D, A, B, 11, 16, 0x6D9D6122);
		P(B, C, D, A, 14, 23, 0xFDE5380C);
		P(A, B, C, D, 1, 4, 0xA4BEEA44);
		P(D, A, B, C, 4, 11, 0x4BDECFA9);
		P(C, D, A, B, 7, 16, 0xF6BB4B60);
		P(B, C, D, A, 10, 23, 0xBEBFBC70);
		P(A, B, C, D, 13, 4, 0x289B7EC6);
		P(D, A, B, C, 0, 11, 0xEAA127FA);
		P(C, D, A, B, 3, 16, 0xD4EF3085);
		P(B, C, D, A, 6, 23, 0x04881D05);
		P(A, B, C, D, 9, 4, 0xD9D4D039);
		P(D, A, B, C, 12, 11, 0xE6DB99E5);
		P(C, D, A, B, 15, 16, 0x1FA27CF8);
		P(B, C, D, A, 2, 23, 0xC4AC5665);

#undef F

#define F(x,y,z) (y ^ (x | ~z))

		P(A, B, C, D, 0, 6, 0xF4292244);
		P(D, A, B, C, 7, 10, 0x432AFF97);
		P(C, D, A, B, 14, 15, 0xAB9423A7);
		P(B, C, D, A, 5, 21, 0xFC93A039);
		P(A, B, C, D, 12, 6, 0x655B59C3);
		P(D, A, B, C, 3, 10, 0x8F0CCC92);
		P(C, D, A, B, 10, 15, 0xFFEFF47D);
		P(B, C, D, A, 1, 21, 0x85845DD1);
		P(A, B, C, D, 8, 6, 0x6FA87E4F);
		P(D, A, B, C, 15, 10, 0xFE2CE6E0);
		P(C, D, A, B, 6, 15, 0xA3014314);
		P(B, C, D, A, 13, 21, 0x4E0811A1);
		P(A, B, C, D, 4, 6, 0xF7537E82);
		P(D, A, B, C, 11, 10, 0xBD3AF235);
		P(C, D, A, B, 2, 15, 0x2AD7D2BB);
		P(B, C, D, A, 9, 21, 0xEB86D391);

#undef F

		this->state[0] += A;
		this->state[1] += B;
		this->state[2] += C;
		this->state[3] += D;
	}

	#define PUT_ULONG_LE(n,b,i)                             \
	{                                                       \
		(b)[(i)    ] = (unsigned char) ( (n)       );       \
		(b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
		(b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
		(b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
	}

	static const unsigned char md5_padding[64] =
	{
	 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	void md5::State::finalise() noexcept
	{
		unsigned long last, padn;
		unsigned long high, low;
		unsigned char msglen[8];

		high = ((uint32_t)n_bytes >> 29)
			| ((n_bytes >> 32) << 3);
		low = ((uint32_t)n_bytes << 3);

		PUT_ULONG_LE(low, msglen, 0);
		PUT_ULONG_LE(high, msglen, 4);

		last = n_bytes & 0x3F;
		padn = (last < 56) ? (56 - last) : (120 - last);

		append((unsigned char*)md5_padding, padn);
		append(msglen, 8);
	}

	void md5::State::getDigest(uint8_t out[DIGEST_BYTES]) const noexcept
	{
		PUT_ULONG_LE(state[0], out, 0);
		PUT_ULONG_LE(state[1], out, 4);
		PUT_ULONG_LE(state[2], out, 8);
		PUT_ULONG_LE(state[3], out, 12);
	}

	std::string md5::State::getDigest() const SOUP_EXCAL
	{
		std::string digest(DIGEST_BYTES, '\0');
		getDigest((uint8_t*)digest.data());
		return digest;
	}
}
