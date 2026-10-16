#include "lookup3.hpp"

NAMESPACE_SOUP
{
	// lookup3 by Bob Jekins, code is public domain.
	// https://burtleburtle.net/bob/c/lookup3.c

	#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

	#define mix(a,b,c) \
	{ \
	  a -= c;  a ^= rot(c, 4);  c += b; \
	  b -= a;  b ^= rot(a, 6);  a += c; \
	  c -= b;  c ^= rot(b, 8);  b += a; \
	  a -= c;  a ^= rot(c,16);  c += b; \
	  b -= a;  b ^= rot(a,19);  a += c; \
	  c -= b;  c ^= rot(b, 4);  b += a; \
	}

	#define final(a,b,c) \
	{ \
	  c ^= b; c -= rot(b,14); \
	  a ^= c; a -= rot(c,11); \
	  b ^= a; b -= rot(a,25); \
	  c ^= b; c -= rot(b,16); \
	  a ^= c; a -= rot(c,4);  \
	  b ^= a; b -= rot(a,14); \
	  c ^= b; c -= rot(b,24); \
	}

	void lookup3::hashlittle2(const void* data, size_t size, uint32_t& pc, uint32_t& pb)
	{
		uint32_t a, b, c; /* internal state */

		/* Set up the internal state */
		a = b = c = 0xdeadbeef + ((uint32_t)size) + pc;
		c += pb;

		const uint32_t* k = (const uint32_t*)data; /* read 32-bit chunks */

		while (size > 12) /* all but last block: aligned reads and affect 32 bits of (a,b,c) */
		{
			a += k[0];
			b += k[1];
			c += k[2];
			mix(a, b, c);
			size -= 12;
			k += 3;
		}

		switch (size)
		{
		case 12:
			c += k[2];
			b += k[1];
			a += k[0];
			break;
		case 11:
			c += k[2] & 0xffffff;
			b += k[1];
			a += k[0];
			break;
		case 10:
			c += k[2] & 0xffff;
			b += k[1];
			a += k[0];
			break;
		case 9:
			c += k[2] & 0xff;
			b += k[1];
			a += k[0];
			break;
		case 8:
			b += k[1];
			a += k[0];
			break;
		case 7:
			b += k[1] & 0xffffff;
			a += k[0];
			break;
		case 6:
			b += k[1] & 0xffff;
			a += k[0];
			break;
		case 5:
			b += k[1] & 0xff;
			a += k[0];
			break;
		case 4:
			a += k[0];
			break;
		case 3:
			a += k[0] & 0xffffff;
			break;
		case 2:
			a += k[0] & 0xffff;
			break;
		case 1:
			a += k[0] & 0xff;
			break;
		case 0:
			/* zero length strings require no mixing */
			pc = c;
			pb = b;
			return;
		}

		final(a, b, c);

		pc = c;
		pb = b;
	}
}
