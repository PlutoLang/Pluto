#include "lzf.hpp"

#include <cstdint>
#include <cstring> // memset

/*
 * Copyright (c) 2000-2010 Marc Alexander Lehmann <schmorp@schmorp.de>
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

NAMESPACE_SOUP
{
	using u8 = uint8_t;
	using u16 = uint16_t;

	static constexpr auto HLOG = 16;
	static constexpr auto HSIZE = 1 << HLOG;

#define FRST(p) (((p[0]) << 8) | p[1])
#define NEXT(v,p) (((v) << 8) | p[2])
#define IDX(h) ((( h             >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))

#define STRICT_ALIGN !SOUP_X86

#define LZF_USE_OFFSETS SOUP_BITS >= 64

#if LZF_USE_OFFSETS
    #define LZF_HSLOT_BIAS ((const u8 *)in_data)
	using LZF_HSLOT = unsigned int;
#else
    #define LZF_HSLOT_BIAS 0
	using LZF_HSLOT = const u8*;
#endif

    using LZF_STATE = LZF_HSLOT[HSIZE];

    static constexpr auto MAX_LIT = (1 <<  5);
    static constexpr auto MAX_OFF = (1 << 13);
    static constexpr auto MAX_REF = ((1 << 8) + (1 << 3));

#if __GNUC__ >= 3
    #define expect(expr,value)         __builtin_expect ((expr),(value))
#else
    #define expect(expr,value)         (expr)
#endif

#define expect_false(expr) expect ((expr) != 0, 0)
#define expect_true(expr)  expect ((expr) != 0, 1)

	unsigned int lzf::compress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len)
	{
        LZF_STATE htab;
        const u8* ip = (const u8*)in_data;
        u8* op = (u8*)out_data;
        const u8* in_end = ip + in_len;
        u8* out_end = op + out_len;
        const u8* ref;

        /* off requires a type wide enough to hold a general pointer difference.
         * ISO C doesn't have that (size_t might not be enough and ptrdiff_t only
         * works for differences within a single object). We also assume that no
         * no bit pattern traps. Since the only platform that is both non-POSIX
         * and fails to support both assumptions is windows 64 bit, we make a
         * special workaround for it.
         */
#if defined (WIN32) && defined (_M_X64)
        unsigned _int64 off; /* workaround for missing POSIX compliance */
#else
        unsigned long off;
#endif
        unsigned int hval;
        int lit;

        if (!in_len || !out_len)
            return 0;

        memset(htab, 0, sizeof(htab));

        lit = 0; op++; /* start run */

        hval = FRST(ip);
        while (ip < in_end - 2)
        {
            LZF_HSLOT* hslot;

            hval = NEXT(hval, ip);
            hslot = htab + IDX(hval);
            ref = *hslot + LZF_HSLOT_BIAS; *hslot = ip - LZF_HSLOT_BIAS;

            if (1
                && ref < ip /* the next test will actually take care of this, but this is faster */
                && (off = ip - ref - 1) < MAX_OFF
                && ref > (u8*)in_data
                && ref[2] == ip[2]
#if STRICT_ALIGN
                && ((ref[1] << 8) | ref[0]) == ((ip[1] << 8) | ip[0])
#else
                && *(u16*)ref == *(u16*)ip
#endif
                )
            {
                /* match found at *ref++ */
                unsigned int len = 2;
                unsigned int maxlen = in_end - ip - len;
                maxlen = maxlen > MAX_REF ? MAX_REF : maxlen;

                if (expect_false(op + 3 + 1 >= out_end)) /* first a faster conservative test */
                    if (op - !lit + 3 + 1 >= out_end) /* second the exact but rare test */
                        return 0;

                op[-lit - 1] = lit - 1; /* stop run */
                op -= !lit; /* undo run if length is zero */

                for (;;)
                {
                    if (expect_true(maxlen > 16))
                    {
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;

                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;

                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;

                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                        len++; if (ref[len] != ip[len]) break;
                    }

                    do
                        len++;
                    while (len < maxlen && ref[len] == ip[len]);

                    break;
                }

                len -= 2; /* len is now #octets - 1 */
                ip++;

                if (len < 7)
                {
                    *op++ = (off >> 8) + (len << 5);
                }
                else
                {
                    *op++ = (off >> 8) + (7 << 5);
                    *op++ = len - 7;
                }

                *op++ = off;

                lit = 0; op++; /* start run */

                ip += len + 1;

                if (expect_false(ip >= in_end - 2))
                    break;

#if ULTRA_FAST || VERY_FAST
                --ip;
# if VERY_FAST && !ULTRA_FAST
                --ip;
# endif
                hval = FRST(ip);

                hval = NEXT(hval, ip);
                htab[IDX(hval)] = ip - LZF_HSLOT_BIAS;
                ip++;

# if VERY_FAST && !ULTRA_FAST
                hval = NEXT(hval, ip);
                htab[IDX(hval)] = ip - LZF_HSLOT_BIAS;
                ip++;
# endif
#else
                ip -= len + 1;

                do
                {
                    hval = NEXT(hval, ip);
                    htab[IDX(hval)] = ip - LZF_HSLOT_BIAS;
                    ip++;
                } while (len--);
#endif
            }
            else
            {
                /* one more literal byte we must copy */
                if (expect_false(op >= out_end))
                    return 0;

                lit++; *op++ = *ip++;

                if (expect_false(lit == MAX_LIT))
                {
                    op[-lit - 1] = lit - 1; /* stop run */
                    lit = 0; op++; /* start run */
                }
            }
        }

        if (op + 3 > out_end) /* at most 3 bytes can be missing here */
            return 0;

        while (ip < in_end)
        {
            lit++; *op++ = *ip++;

            if (expect_false(lit == MAX_LIT))
            {
                op[-lit - 1] = lit - 1; /* stop run */
                lit = 0; op++; /* start run */
            }
        }

        op[-lit - 1] = lit - 1; /* end run */
        op -= !lit; /* undo run if length is zero */

        return op - (u8*)out_data;
	}

#undef FRST
#undef NEXT
#undef IDX
#undef STRICT_ALIGN
#undef LZF_USE_OFFSETS
#undef LZF_HSLOT_BIAS
#undef expect
#undef expect_false
#undef expect_true

	unsigned int lzf::decompress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len)
	{
		u8 const* ip = (const u8*)in_data;
		u8* op = (u8*)out_data;
		u8 const* const in_end = ip + in_len;
		u8* const out_end = op + out_len;

		do
		{
			unsigned int ctrl = *ip++;

			if (ctrl < (1 << 5)) /* literal run */
			{
				ctrl++;

				if (op + ctrl > out_end)
				{
					// output buffer is too small
					return 0;
				}

				/*if (ip + ctrl > in_end)
				{
					return 0;
				}*/

				switch (ctrl)
				{
				case 32: *op++ = *ip++; [[fallthrough]]; case 31: *op++ = *ip++; [[fallthrough]]; case 30: *op++ = *ip++; [[fallthrough]]; case 29: *op++ = *ip++; [[fallthrough]];
				case 28: *op++ = *ip++; [[fallthrough]]; case 27: *op++ = *ip++; [[fallthrough]]; case 26: *op++ = *ip++; [[fallthrough]]; case 25: *op++ = *ip++; [[fallthrough]];
				case 24: *op++ = *ip++; [[fallthrough]]; case 23: *op++ = *ip++; [[fallthrough]]; case 22: *op++ = *ip++; [[fallthrough]]; case 21: *op++ = *ip++; [[fallthrough]];
				case 20: *op++ = *ip++; [[fallthrough]]; case 19: *op++ = *ip++; [[fallthrough]]; case 18: *op++ = *ip++; [[fallthrough]]; case 17: *op++ = *ip++; [[fallthrough]];
				case 16: *op++ = *ip++; [[fallthrough]]; case 15: *op++ = *ip++; [[fallthrough]]; case 14: *op++ = *ip++; [[fallthrough]]; case 13: *op++ = *ip++; [[fallthrough]];
				case 12: *op++ = *ip++; [[fallthrough]]; case 11: *op++ = *ip++; [[fallthrough]]; case 10: *op++ = *ip++; [[fallthrough]]; case  9: *op++ = *ip++; [[fallthrough]];
				case  8: *op++ = *ip++; [[fallthrough]]; case  7: *op++ = *ip++; [[fallthrough]]; case  6: *op++ = *ip++; [[fallthrough]]; case  5: *op++ = *ip++; [[fallthrough]];
				case  4: *op++ = *ip++; [[fallthrough]]; case  3: *op++ = *ip++; [[fallthrough]]; case  2: *op++ = *ip++; [[fallthrough]]; case  1: *op++ = *ip++;
				}
			}
			else /* back reference */
			{
				unsigned int len = ctrl >> 5;

				u8* ref = op - ((ctrl & 0x1f) << 8) - 1;

				/*if (ip >= in_end)
				{
					return 0;
				}*/
				if (len == 7)
				{
					len += *ip++;
					/*if (ip >= in_end)
					{
						return 0;
					}*/
				}

				ref -= *ip++;

				if (op + len + 2 > out_end)
				{
					// output buffer is too small
					return 0;
				}

				if (ref < (u8*)out_data)
				{
					// undecompressable
					return 0;
				}

				switch (len)
				{
				default:
					len += 2;

					if (op >= ref + len)
					{
						/* disjunct areas */
						memcpy(op, ref, len);
						op += len;
					}
					else
					{
						/* overlapping, use octte by octte copying */
						do
							*op++ = *ref++;
						while (--len);
					}

					break;

				case 9: *op++ = *ref++; [[fallthrough]];
				case 8: *op++ = *ref++; [[fallthrough]];
				case 7: *op++ = *ref++; [[fallthrough]];
				case 6: *op++ = *ref++; [[fallthrough]];
				case 5: *op++ = *ref++; [[fallthrough]];
				case 4: *op++ = *ref++; [[fallthrough]];
				case 3: *op++ = *ref++; [[fallthrough]];
				case 2: *op++ = *ref++; [[fallthrough]];
				case 1: *op++ = *ref++; [[fallthrough]];
				case 0: *op++ = *ref++; /* two octets more */
					*op++ = *ref++;
				}
			}
		} while (ip < in_end);

		return op - (u8*)out_data;
	}
}
