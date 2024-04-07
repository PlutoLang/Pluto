#include "Curve25519.hpp"

#include <cstring> // memcpy

#include "rand.hpp"

NAMESPACE_SOUP
{
#define F25519_SIZE 32

	static const uint8_t f25519_one[F25519_SIZE] = { 1 };
	const uint8_t c25519_base_x[F25519_SIZE] = { 9 };

	/* Having generated 32 random bytes, you should call this function to
	 * finalize the generated key.
	 */
	static inline void c25519_prepare(uint8_t* key)
	{
		key[0] &= 0xf8;
		key[31] &= 0x7f;
		key[31] |= 0x40;
	}

	/* Copy two points */
	static inline void f25519_copy(uint8_t* x, const uint8_t* a)
	{
		memcpy(x, a, F25519_SIZE);
	}

	static void f25519_select(uint8_t* dst,
		const uint8_t* zero, const uint8_t* one,
		uint8_t condition)
	{
		const uint8_t mask = -condition;
		int i;

		for (i = 0; i < F25519_SIZE; i++)
			dst[i] = zero[i] ^ (mask & (one[i] ^ zero[i]));
	}

	static void f25519_mul__distinct(uint8_t* r, const uint8_t* a, const uint8_t* b)
	{
		uint32_t c = 0;
		int i;

		for (i = 0; i < F25519_SIZE; i++) {
			int j;

			c >>= 8;
			for (j = 0; j <= i; j++)
				c += ((uint32_t)a[j]) * ((uint32_t)b[i - j]);

			for (; j < F25519_SIZE; j++)
				c += ((uint32_t)a[j]) *
				((uint32_t)b[i + F25519_SIZE - j]) * 38;

			r[i] = c;
		}

		r[31] &= 127;
		c = (c >> 7) * 19;

		for (i = 0; i < F25519_SIZE; i++) {
			c += r[i];
			r[i] = c;
			c >>= 8;
		}
	}

	static void f25519_inv__distinct(uint8_t* r, const uint8_t* x)
	{
		uint8_t s[F25519_SIZE];
		int i;

		/* This is a prime field, so by Fermat's little theorem:
		 *
		 *     x^(p-1) = 1 mod p
		 *
		 * Therefore, raise to (p-2) = 2^255-21 to get a multiplicative
		 * inverse.
		 *
		 * This is a 255-bit binary number with the digits:
		 *
		 *     11111111... 01011
		 *
		 * We compute the result by the usual binary chain, but
		 * alternate between keeping the accumulator in r and s, so as
		 * to avoid copying temporaries.
		 */

		 /* 1 1 */
		f25519_mul__distinct(s, x, x);
		f25519_mul__distinct(r, s, x);

		/* 1 x 248 */
		for (i = 0; i < 248; i++) {
			f25519_mul__distinct(s, r, r);
			f25519_mul__distinct(r, s, x);
		}

		/* 0 */
		f25519_mul__distinct(s, r, r);

		/* 1 */
		f25519_mul__distinct(r, s, s);
		f25519_mul__distinct(s, r, x);

		/* 0 */
		f25519_mul__distinct(r, s, s);

		/* 1 */
		f25519_mul__distinct(s, r, r);
		f25519_mul__distinct(r, s, x);

		/* 1 */
		f25519_mul__distinct(s, r, r);
		f25519_mul__distinct(r, s, x);
	}

	static void f25519_normalize(uint8_t* x)
	{
		uint8_t minusp[F25519_SIZE];
		uint16_t c;
		int i;

		/* Reduce using 2^255 = 19 mod p */
		c = (x[31] >> 7) * 19;
		x[31] &= 127;

		for (i = 0; i < F25519_SIZE; i++) {
			c += x[i];
			x[i] = static_cast<uint8_t>(c);
			c >>= 8;
		}

		/* The number is now less than 2^255 + 18, and therefore less than
		 * 2p. Try subtracting p, and conditionally load the subtracted
		 * value if underflow did not occur.
		 */
		c = 19;

		for (i = 0; i + 1 < F25519_SIZE; i++) {
			c += x[i];
			minusp[i] = static_cast<uint8_t>(c);
			c >>= 8;
		}

		c += ((uint16_t)x[i]) - 128;
		minusp[31] = static_cast<uint8_t>(c);

		/* Load x-p if no underflow */
		f25519_select(x, minusp, x, (c >> 15) & 1);
	}

	static void f25519_add(uint8_t* r, const uint8_t* a, const uint8_t* b)
	{
		uint16_t c = 0;
		int i;

		/* Add */
		for (i = 0; i < F25519_SIZE; i++) {
			c >>= 8;
			c += ((uint16_t)a[i]) + ((uint16_t)b[i]);
			r[i] = static_cast<uint8_t>(c);
		}

		/* Reduce with 2^255 = 19 mod p */
		r[31] &= 127;
		c = (c >> 7) * 19;

		for (i = 0; i < F25519_SIZE; i++) {
			c += r[i];
			r[i] = static_cast<uint8_t>(c);
			c >>= 8;
		}
	}

	static void f25519_sub(uint8_t* r, const uint8_t* a, const uint8_t* b)
	{
		uint32_t c = 0;
		int i;

		/* Calculate a + 2p - b, to avoid underflow */
		c = 218;
		for (i = 0; i + 1 < F25519_SIZE; i++) {
			c += 65280 + ((uint32_t)a[i]) - ((uint32_t)b[i]);
			r[i] = c;
			c >>= 8;
		}

		c += ((uint32_t)a[31]) - ((uint32_t)b[31]);
		r[31] = c & 127;
		c = (c >> 7) * 19;

		for (i = 0; i < F25519_SIZE; i++) {
			c += r[i];
			r[i] = c;
			c >>= 8;
		}
	}

	/* Differential addition */
	static void xc_diffadd(uint8_t* x5, uint8_t* z5,
		const uint8_t* x1, const uint8_t* z1,
		const uint8_t* x2, const uint8_t* z2,
		const uint8_t* x3, const uint8_t* z3)
	{
		/* Explicit formulas database: dbl-1987-m3
		 *
		 * source 1987 Montgomery "Speeding the Pollard and elliptic curve
		 *   methods of factorization", page 261, fifth display, plus
		 *   common-subexpression elimination
		 * compute A = X2+Z2
		 * compute B = X2-Z2
		 * compute C = X3+Z3
		 * compute D = X3-Z3
		 * compute DA = D A
		 * compute CB = C B
		 * compute X5 = Z1(DA+CB)^2
		 * compute Z5 = X1(DA-CB)^2
		 */
		uint8_t da[F25519_SIZE];
		uint8_t cb[F25519_SIZE];
		uint8_t a[F25519_SIZE];
		uint8_t b[F25519_SIZE];

		f25519_add(a, x2, z2);
		f25519_sub(b, x3, z3); /* D */
		f25519_mul__distinct(da, a, b);

		f25519_sub(b, x2, z2);
		f25519_add(a, x3, z3); /* C */
		f25519_mul__distinct(cb, a, b);

		f25519_add(a, da, cb);
		f25519_mul__distinct(b, a, a);
		f25519_mul__distinct(x5, z1, b);

		f25519_sub(a, da, cb);
		f25519_mul__distinct(b, a, a);
		f25519_mul__distinct(z5, x1, b);
	}

	static void f25519_mul_c(uint8_t* r, const uint8_t* a, uint32_t b)
	{
		uint32_t c = 0;
		int i;

		for (i = 0; i < F25519_SIZE; i++) {
			c >>= 8;
			c += b * ((uint32_t)a[i]);
			r[i] = c;
		}

		r[31] &= 127;
		c >>= 7;
		c *= 19;

		for (i = 0; i < F25519_SIZE; i++) {
			c += r[i];
			r[i] = c;
			c >>= 8;
		}
	}

	/* Double an X-coordinate */
	static void xc_double(uint8_t* x3, uint8_t* z3,
		const uint8_t* x1, const uint8_t* z1)
	{
		/* Explicit formulas database: dbl-1987-m
		 *
		 * source 1987 Montgomery "Speeding the Pollard and elliptic
		 *   curve methods of factorization", page 261, fourth display
		 * compute X3 = (X1^2-Z1^2)^2
		 * compute Z3 = 4 X1 Z1 (X1^2 + a X1 Z1 + Z1^2)
		 */
		uint8_t x1sq[F25519_SIZE];
		uint8_t z1sq[F25519_SIZE];
		uint8_t x1z1[F25519_SIZE];
		uint8_t a[F25519_SIZE];

		f25519_mul__distinct(x1sq, x1, x1);
		f25519_mul__distinct(z1sq, z1, z1);
		f25519_mul__distinct(x1z1, x1, z1);

		f25519_sub(a, x1sq, z1sq);
		f25519_mul__distinct(x3, a, a);

		f25519_mul_c(a, x1z1, 486662);
		f25519_add(a, x1sq, a);
		f25519_add(a, z1sq, a);
		f25519_mul__distinct(x1sq, x1z1, a);
		f25519_mul_c(z3, x1sq, 4);
	}

	void c25519_smult(uint8_t* result, const uint8_t* q, const uint8_t* e)
	{
		/* Current point: P_m */
		uint8_t xm[F25519_SIZE];
		uint8_t zm[F25519_SIZE] = { 1 };

		/* Predecessor: P_(m-1) */
		uint8_t xm1[F25519_SIZE] = { 1 };
		uint8_t zm1[F25519_SIZE] = { 0 };

		int i;

		/* Note: bit 254 is assumed to be 1 */
		f25519_copy(xm, q);

		for (i = 253; i >= 0; i--) {
			const int bit = (e[i >> 3] >> (i & 7)) & 1;
			uint8_t xms[F25519_SIZE];
			uint8_t zms[F25519_SIZE];

			/* From P_m and P_(m-1), compute P_(2m) and P_(2m-1) */
			xc_diffadd(xm1, zm1, q, f25519_one, xm, zm, xm1, zm1);
			xc_double(xm, zm, xm, zm);

			/* Compute P_(2m+1) */
			xc_diffadd(xms, zms, xm1, zm1, xm, zm, q, f25519_one);

			/* Select:
			 *   bit = 1 --> (P_(2m+1), P_(2m))
			 *   bit = 0 --> (P_(2m), P_(2m-1))
			 */
			f25519_select(xm1, xm1, xm, bit);
			f25519_select(zm1, zm1, zm, bit);
			f25519_select(xm, xm, xms, bit);
			f25519_select(zm, zm, zms, bit);
		}

		/* Freeze out of projective coordinates */
		f25519_inv__distinct(zm1, zm);
		f25519_mul__distinct(result, zm1, xm);
		f25519_normalize(result);
	}

	void Curve25519::generatePrivate(uint8_t(&private_key)[KEY_SIZE])
	{
		rand.fill(private_key);
		c25519_prepare(private_key);
	}

	void Curve25519::derivePublic(uint8_t* public_key, const uint8_t* private_key)
	{
		c25519_smult(public_key, c25519_base_x, private_key);
	}

	void Curve25519::x25519(uint8_t(&shared_secret)[SHARED_SIZE], const uint8_t(&my_private_key)[KEY_SIZE], const uint8_t(&their_public_key)[KEY_SIZE])
	{
		uint8_t clamped_private_key[KEY_SIZE];
		memcpy(clamped_private_key, my_private_key, KEY_SIZE);
		c25519_prepare(clamped_private_key);
		c25519_smult(shared_secret, their_public_key, clamped_private_key);
	}
}
