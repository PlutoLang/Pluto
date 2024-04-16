#include "adler32.hpp"

// Original source: Some zlib release somewhere.
// Original licence follows.
/*
  Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
*/

#define BASE 65521U
#define NMAX 5552

#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);
#define MOD(a) a %= BASE
#define MOD28(a) a %= BASE
#define MOD63(a) a %= BASE

NAMESPACE_SOUP
{
	uint32_t adler32::hash(const std::string& data)
	{
		return hash(data.data(), data.size());
	}

	uint32_t adler32::hash(const char* data, size_t size)
	{
		return hash((const uint8_t*)data, size);
	}

	uint32_t adler32::hash(const uint8_t* data, size_t size, uint32_t init)
	{
		/* split Adler-32 into component sums */
		uint32_t sum2 = ((init >> 16) & 0xffff);
		uint32_t adler = (init & 0xffff);

		/* in case user likes doing a byte at a time, keep it fast */
		if (size == 1)
		{
			adler += data[0];
			if (adler >= BASE)
			{
				adler -= BASE;
			}
			sum2 += adler;
			if (sum2 >= BASE)
			{
				sum2 -= BASE;
			}
			return adler | (sum2 << 16);
		}

		/* in case short lengths are provided, keep it somewhat fast */
		if (size < 16)
		{
			while (size--)
			{
				adler += *data++;
				sum2 += adler;
			}
			if (adler >= BASE)
			{
				adler -= BASE;
			}
			MOD28(sum2);            /* only added so many BASE's */
			return adler | (sum2 << 16);
		}

		/* do length NMAX blocks -- requires just one modulo operation */
		while (size >= NMAX)
		{
			size -= NMAX;
			uint32_t n = NMAX / 16;          /* NMAX is divisible by 16 */
			do
			{
				DO16(data);          /* 16 sums unrolled */
				data += 16;
			} while (--n);
			MOD(adler);
			MOD(sum2);
		}

		/* do remaining bytes (less than NMAX, still just one modulo) */
		if (size) /* avoid modulos if none remaining */
		{
			while (size >= 16)
			{
				size -= 16;
				DO16(data);
				data += 16;
			}
			while (size--)
			{
				adler += *data++;
				sum2 += adler;
			}
			MOD(adler);
			MOD(sum2);
		}

		/* return recombined sums */
		return adler | (sum2 << 16);
	}
}
