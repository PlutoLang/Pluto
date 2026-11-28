#include "deflate.hpp"

#include <cstring> // memcpy

#include "base.hpp"

#include "adler32.hpp"
#include "BitWriter.hpp"
#include "crc32.hpp"
#include "Endian.hpp"
#include "huffman.hpp"
#include "Lz77Compressor.hpp"
#include "memory.hpp" // construct_at
#include "MemoryRefReader.hpp"
#include "StringWriter.hpp"
#include "utility.hpp" // SOUP_UNREACHABLE

/*
Original source: https://github.com/Artexety/inflatecpp
Original licence follows.

Copyright (c) 2020 Artexety

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#if SOUP_BITS >= 64
#define X64BIT_SHIFTER
#endif

NAMESPACE_SOUP
{
#ifdef X64BIT_SHIFTER
	using shifter_t = uint64_t;
#else
	using shifter_t = uint32_t;
#endif

	#define MATCHLEN_PAIR(__base,__dispbits) ((__base) | ((__dispbits) << 16) | 0x8000)
	#define OFFSET_PAIR(__base,__dispbits) ((__base) | ((__dispbits) << 16))

	constexpr auto kCodeLenBits = 3;
	constexpr auto kLiteralSyms = 288;
	constexpr auto kEODMarkerSym = 256;
	constexpr auto kMatchLenSymStart = 257;
	constexpr auto kMatchLenSyms = 29;
	constexpr auto kOffsetSyms = 32;
	constexpr auto kMinMatchSize = 3;

	constexpr unsigned int kMatchLenCode[kMatchLenSyms] = {
	   MATCHLEN_PAIR(kMinMatchSize + 0, 0), MATCHLEN_PAIR(kMinMatchSize + 1, 0), MATCHLEN_PAIR(kMinMatchSize + 2, 0), MATCHLEN_PAIR(kMinMatchSize + 3, 0), MATCHLEN_PAIR(kMinMatchSize + 4, 0),
	   MATCHLEN_PAIR(kMinMatchSize + 5, 0), MATCHLEN_PAIR(kMinMatchSize + 6, 0), MATCHLEN_PAIR(kMinMatchSize + 7, 0), MATCHLEN_PAIR(kMinMatchSize + 8, 1), MATCHLEN_PAIR(kMinMatchSize + 10, 1),
	   MATCHLEN_PAIR(kMinMatchSize + 12, 1), MATCHLEN_PAIR(kMinMatchSize + 14, 1), MATCHLEN_PAIR(kMinMatchSize + 16, 2), MATCHLEN_PAIR(kMinMatchSize + 20, 2), MATCHLEN_PAIR(kMinMatchSize + 24, 2),
	   MATCHLEN_PAIR(kMinMatchSize + 28, 2), MATCHLEN_PAIR(kMinMatchSize + 32, 3), MATCHLEN_PAIR(kMinMatchSize + 40, 3), MATCHLEN_PAIR(kMinMatchSize + 48, 3), MATCHLEN_PAIR(kMinMatchSize + 56, 3),
	   MATCHLEN_PAIR(kMinMatchSize + 64, 4), MATCHLEN_PAIR(kMinMatchSize + 80, 4), MATCHLEN_PAIR(kMinMatchSize + 96, 4), MATCHLEN_PAIR(kMinMatchSize + 112, 4), MATCHLEN_PAIR(kMinMatchSize + 128, 5),
	   MATCHLEN_PAIR(kMinMatchSize + 160, 5), MATCHLEN_PAIR(kMinMatchSize + 192, 5), MATCHLEN_PAIR(kMinMatchSize + 224, 5), MATCHLEN_PAIR(kMinMatchSize + 255, 0),
	};

	constexpr unsigned int kOffsetCode[kOffsetSyms] = {
	   OFFSET_PAIR(1, 0), OFFSET_PAIR(2, 0), OFFSET_PAIR(3, 0), OFFSET_PAIR(4, 0), OFFSET_PAIR(5, 1), OFFSET_PAIR(7, 1), OFFSET_PAIR(9, 2), OFFSET_PAIR(13, 2), OFFSET_PAIR(17, 3), OFFSET_PAIR(25, 3),
	   OFFSET_PAIR(33, 4), OFFSET_PAIR(49, 4), OFFSET_PAIR(65, 5), OFFSET_PAIR(97, 5), OFFSET_PAIR(129, 6), OFFSET_PAIR(193, 6), OFFSET_PAIR(257, 7), OFFSET_PAIR(385, 7), OFFSET_PAIR(513, 8), OFFSET_PAIR(769, 8),
	   OFFSET_PAIR(1025, 9), OFFSET_PAIR(1537, 9), OFFSET_PAIR(2049, 10), OFFSET_PAIR(3073, 10), OFFSET_PAIR(4097, 11), OFFSET_PAIR(6145, 11), OFFSET_PAIR(8193, 12), OFFSET_PAIR(12289, 12), OFFSET_PAIR(16385, 13), OFFSET_PAIR(24577, 13),
	};

	class DeflateBitReader
	{
	private:
		int shifter_bit_count_ = 0;
		shifter_t shifter_data_ = 0;
		const uint8_t* in_block_;
		const uint8_t* in_block_end_;
		const uint8_t* in_block_start_;

	public:
		/**
		 * Initialize bit reader
		 *
		 * @param in_block pointer to the start of the compressed block
		 * @param in_block_end pointer to the end of the compressed block + 1
		 */
		explicit DeflateBitReader(const uint8_t* in_block, const uint8_t* in_block_end) noexcept
			: in_block_(in_block), in_block_end_(in_block_end), in_block_start_(in_block)
		{
		}

		/** Refill 32 bits at a time if the architecture allows it, otherwise do nothing. */
		void refill32() noexcept
		{
#if SOUP_BITS == 64
			if (this->shifter_bit_count_ <= 32 && (this->in_block_ + 4) <= this->in_block_end_)
			{
				if constexpr (ENDIAN_NATIVE == ENDIAN_LITTLE)
				{
					this->shifter_data_ |= (((shifter_t)(*((unsigned int*)this->in_block_))) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 32;
					this->in_block_ += 4;
				}
				else
				{
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 8;
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 8;
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 8;
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 8;
				}
			}
#endif
		}

		/**
		 * Consume variable bit-length value, after reading it with PeekBits()
		 *
		 * @param n size of value to consume, in bits
		 */
		void consumeBits(const int n) noexcept
		{
			this->shifter_data_ >>= n;
			this->shifter_bit_count_ -= n;
		}

		void modifyInBlock(const int v) noexcept
		{
			this->in_block_ += v;
		}

		/**
		 * Read variable bit-length value
		 *
		 * @param n size of value in bits (number of bits to read), 0..16
		 *
		 * @return value, or -1 for failure
		 */
		[[nodiscard]] unsigned int getBits(const int n) noexcept
		{
			if (this->shifter_bit_count_ < n)
			{
				SOUP_IF_LIKELY (this->in_block_ < this->in_block_end_)
				{
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					this->shifter_bit_count_ += 8;

					if (this->in_block_ < this->in_block_end_)
					{
						this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
						this->shifter_bit_count_ += 8;
					}
				}
				else
				{
					return -1;
				}
			}

			unsigned int value = this->shifter_data_ & ((1 << n) - 1);
			this->shifter_data_ >>= n;
			this->shifter_bit_count_ -= n;
			return value;
		}

		/**
		 * Peek at a 16-bit value in the bitstream (lookahead)
		 *
		 * @return value
		 */
		[[nodiscard]] unsigned int peekBits() noexcept
		{
			if (this->shifter_bit_count_ < 16)
			{
				if (this->in_block_ < this->in_block_end_)
				{
					this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
					if (this->in_block_ < this->in_block_end_)
						this->shifter_data_ |= (((shifter_t)(*this->in_block_++)) << (this->shifter_bit_count_ + 8));
					this->shifter_bit_count_ += 16;
				}
			}

			return this->shifter_data_ & 0xffff;
		}

		/** Re-align bitstream on a byte */
		bool alignToByte() noexcept
		{
			while (this->shifter_bit_count_ >= 8)
			{
				this->shifter_bit_count_ -= 8;
				this->in_block_--;
				if (this->in_block_ < this->in_block_start_)
				{
					return false;
				}
			}

			this->shifter_bit_count_ = 0;
			this->shifter_data_ = 0;
			return true;
		}

		[[nodiscard]] const uint8_t* getInBlock() noexcept { return this->in_block_; };
		[[nodiscard]] const uint8_t* getInBlockEnd() noexcept { return this->in_block_end_; };
		[[nodiscard]] const uint8_t* getInBlockStart() noexcept { return this->in_block_start_; };
	};
	static_assert(sizeof(deflate::Context) >= sizeof(DeflateBitReader));
	static_assert(std::is_trivially_destructible_v<DeflateBitReader>);

	constexpr auto kMaxSymbols = 288;
	constexpr auto kCodeLenSyms = 19;
	constexpr auto kFastSymbolBits = 10;

	static const uint8_t code_len_syms[kCodeLenSyms] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

	class HuffmanDecoder
	{
	private:
		unsigned int fast_symbol_[1 << kFastSymbolBits];
		unsigned int start_index_[16];
		unsigned int symbols_;
		int num_sorted_;
		int starting_pos_[16];

	public:
		/**
		 * Prepare huffman tables
		 *
		 * @param rev_symbol_table array of 2 * symbols entries for storing the reverse lookup table
		 * @param code_length codeword lengths table
		 */
		bool prepareTable(unsigned int* rev_symbol_table, const int read_symbols, const int symbols, unsigned char* code_length) noexcept
		{
			int num_symbols_per_len[16];
			int i;

			if (read_symbols < 0 || read_symbols > kMaxSymbols || symbols < 0 || symbols > kMaxSymbols || read_symbols > symbols)
			{
				return false;
			}
			this->symbols_ = symbols;


			for (i = 0; i < 16; ++i)
				num_symbols_per_len[i] = 0;

			for (i = 0; i < read_symbols; ++i)
			{
				if (code_length[i] >= 16)
				{
					return false;
				}
				num_symbols_per_len[code_length[i]]++;
			}

			this->starting_pos_[0] = 0;
			this->num_sorted_ = 0;
			for (i = 1; i != 16; ++i)
			{
				this->starting_pos_[i] = this->num_sorted_;
				this->num_sorted_ += num_symbols_per_len[i];
			}

			for (i = 0; i < symbols; ++i)
				rev_symbol_table[i] = -1;

			for (i = 0; i < read_symbols; ++i)
			{
				if (code_length[i])
					rev_symbol_table[this->starting_pos_[code_length[i]]++] = i;
			}

			return true;
		}

		/**
		 * Finalize huffman codewords for decoding
		 *
		 * @param rev_symbol_table array of 2 * symbols entries that contains the reverse lookup table
		 */
		bool finaliseTable(unsigned int* rev_symbol_table) noexcept
		{
			const int symbols = this->symbols_;
			unsigned int canonical_code_word = 0;
			unsigned int* rev_code_length_table = rev_symbol_table + symbols;
			int canonical_length = 1;
			int i;

			for (i = 0; i != (1 << kFastSymbolBits); ++i)
			{
				this->fast_symbol_[i] = 0;
			}
			for (i = 0; i != 16; ++i)
			{
				this->start_index_[i] = 0;
			}

			i = 0;
			while (i < this->num_sorted_)
			{
				if (canonical_length >= 16)
				{
					return false;
				}
				this->start_index_[canonical_length] = i - canonical_code_word;

				while (i < this->starting_pos_[canonical_length])
				{
					if (i >= symbols)
					{
						return false;
					}
					rev_code_length_table[i] = canonical_length;

					if (canonical_code_word >= (1U << canonical_length))
					{
						return false;
					}

					if (canonical_length <= kFastSymbolBits)
					{
						unsigned int rev_word;

						/* Get upside down codeword (branchless method by Eric Biggers) */
						rev_word = ((canonical_code_word & 0x5555) << 1) | ((canonical_code_word & 0xaaaa) >> 1);
						rev_word = ((rev_word & 0x3333) << 2) | ((rev_word & 0xcccc) >> 2);
						rev_word = ((rev_word & 0x0f0f) << 4) | ((rev_word & 0xf0f0) >> 4);
						rev_word = ((rev_word & 0x00ff) << 8) | ((rev_word & 0xff00) >> 8);
						rev_word = rev_word >> (16 - canonical_length);

						int slots = 1 << (kFastSymbolBits - canonical_length);
						while (slots)
						{
							this->fast_symbol_[rev_word] = (rev_symbol_table[i] & 0xffffff) | (canonical_length << 24);
							rev_word += (1 << canonical_length);
							slots--;
						}
					}

					i++;
					canonical_code_word++;
				}
				canonical_length++;
				canonical_code_word <<= 1;
			}

			while (i < symbols)
			{
				rev_symbol_table[i] = -1;
				rev_code_length_table[i++] = 0;
			}

			return true;
		}

		/**
		 * Read fixed bit size code lengths
		 *
		 * @param len_bits number of bits per code length
		 * @param read_symbols number of symbols actually read
		 * @param symbols number of symbols to build codes for
		 * @param code_length output code lengths table
		 * @param bit_reader bit reader context
		 */
		static bool readRawLengths(const int len_bits, const int read_symbols, const int symbols, unsigned char* code_length, DeflateBitReader& bit_reader) noexcept
		{
			int i;

			if (read_symbols < 0 || read_symbols > kMaxSymbols || symbols < 0 || symbols > kMaxSymbols || read_symbols > symbols)
				return false;

			i = 0;
			while (i < read_symbols)
			{
				unsigned int length = bit_reader.getBits(len_bits);
				if (length == -1)
					return false;

				code_length[code_len_syms[i++]] = length;
			}

			while (i < symbols)
			{
				code_length[code_len_syms[i++]] = 0;
			}

			return true;
		}

		/**
		 * Read huffman-encoded code lengths
		 *
		 * @param tables_rev_symbol_table reverse lookup table for code lengths
		 * @param read_symbols number of symbols actually read
		 * @param symbols number of symbols to build codes for
		 * @param code_length output code lengths table
		 * @param bit_reader bit reader context
		 */
		bool readLength(const unsigned int* tables_rev_symbol_table, const int read_symbols, const int symbols, unsigned char* code_length, DeflateBitReader& bit_reader) noexcept
		{
			int i;
			if (read_symbols < 0 || symbols < 0 || read_symbols > symbols)
				return false;

			i = 0;
			unsigned int previous_length = 0;

			while (i < read_symbols)
			{
				unsigned int length = this->readValue(tables_rev_symbol_table, bit_reader);
				if (length == -1)
					return false;

				if (length < 16)
				{
					previous_length = length;
					code_length[i++] = previous_length;
				}
				else
				{
					unsigned int run_length = 0;

					if (length == 16)
					{
						int extra_run_length = bit_reader.getBits(2);
						if (extra_run_length == -1)
							return false;
						run_length = 3 + extra_run_length;
					}
					else if (length == 17)
					{
						int extra_run_length = bit_reader.getBits(3);
						if (extra_run_length == -1)
							return false;
						previous_length = 0;
						run_length = 3 + extra_run_length;
					}
					else if (length == 18)
					{
						int extra_run_length = bit_reader.getBits(7);
						if (extra_run_length == -1)
							return false;
						previous_length = 0;
						run_length = 11 + extra_run_length;
					}

					while (run_length && i < read_symbols)
					{
						code_length[i++] = previous_length;
						run_length--;
					}
				}
			}

			while (i < symbols)
				code_length[i++] = 0;
			return true;
		}

		/**
		 * Decode next symbol
		 *
		 * @param rev_symbol_table reverse lookup table
		 * @param bit_reader bit reader context
		 *
		 * @return symbol, or -1 for error
		 */
		unsigned int readValue(const unsigned int* rev_symbol_table, DeflateBitReader& bit_reader) noexcept
		{
			unsigned int stream = bit_reader.peekBits();
			unsigned int fast_sym_bits = this->fast_symbol_[stream & ((1 << kFastSymbolBits) - 1)];
			if (fast_sym_bits)
			{
				bit_reader.consumeBits(fast_sym_bits >> 24);
				return fast_sym_bits & 0xffffff;
			}
			const unsigned int* rev_code_length_table = rev_symbol_table + this->symbols_;
			unsigned int code_word = 0;
			int bits = 1;

			do
			{
				code_word |= (stream & 1);

				unsigned int table_index = this->start_index_[bits] + code_word;
				if (table_index < this->symbols_)
				{
					if (bits == rev_code_length_table[table_index])
					{
						bit_reader.consumeBits(bits);
						return rev_symbol_table[table_index];
					}
				}
				code_word <<= 1;
				stream >>= 1;
				bits++;
			} while (bits < 16);

			return -1;
		}
	};

	static unsigned int copy_stored(DeflateBitReader& bit_reader, unsigned char* out, size_t out_offset, size_t block_size_max) noexcept
	{
		SOUP_IF_UNLIKELY (!bit_reader.alignToByte())
		{
			return -1;
		}

		SOUP_IF_UNLIKELY ((bit_reader.getInBlock() + 4) > bit_reader.getInBlockEnd())
		{
			return -1;
		}

		uint16_t stored_length = ((unsigned short)bit_reader.getInBlock()[0]) | (((unsigned short)bit_reader.getInBlock()[1]) << 8);
		bit_reader.modifyInBlock(2);

		uint16_t neg_stored_length = ((unsigned short)bit_reader.getInBlock()[0]) | (((unsigned short)bit_reader.getInBlock()[1]) << 8);
		bit_reader.modifyInBlock(2);

		SOUP_IF_UNLIKELY (stored_length != (uint16_t)~neg_stored_length)
		{
			return -1;
		}

		SOUP_IF_UNLIKELY (stored_length > block_size_max)
		{
			return -1;
		}

		memcpy(out + out_offset, bit_reader.getInBlock(), stored_length);
		bit_reader.modifyInBlock(stored_length);

		return stored_length;
	}

	static unsigned int decompress_block(DeflateBitReader& br, bool dynamic_block, unsigned char* out, size_t out_offset, size_t block_size_max) noexcept
	{
		HuffmanDecoder literals_decoder;
		HuffmanDecoder offset_decoder;
		unsigned int literals_rev_sym_table[kLiteralSyms * 2];
		unsigned int offset_rev_sym_table[kLiteralSyms * 2];
		int i;

		if (dynamic_block)
		{
			HuffmanDecoder tables_decoder;
			unsigned char code_length[kLiteralSyms + kOffsetSyms];
			unsigned int tables_rev_sym_table[kCodeLenSyms * 2];

			unsigned int literal_syms = br.getBits(5);
			SOUP_IF_UNLIKELY (literal_syms == -1)
			{
				return -1;
			}
			literal_syms += 257;
			SOUP_IF_UNLIKELY (literal_syms > kLiteralSyms)
			{
				return -1;
			}

			unsigned int offset_syms = br.getBits(5);
			SOUP_IF_UNLIKELY (offset_syms == -1)
			{
				return -1;
			}
			offset_syms += 1;
			SOUP_IF_UNLIKELY (offset_syms > kOffsetSyms)
			{
				return -1;
			}

			unsigned int code_len_syms = br.getBits(4);
			SOUP_IF_UNLIKELY (code_len_syms == -1)
			{
				return -1;
			}
			code_len_syms += 4;
			SOUP_IF_UNLIKELY (code_len_syms > kCodeLenSyms)
			{
				return -1;
			}

			SOUP_IF_UNLIKELY (!HuffmanDecoder::readRawLengths(kCodeLenBits, code_len_syms, kCodeLenSyms, code_length, br))
			{
				return -1;
			}
			SOUP_IF_UNLIKELY (!tables_decoder.prepareTable(tables_rev_sym_table, kCodeLenSyms, kCodeLenSyms, code_length))
			{
				return -1;
			}
			SOUP_IF_UNLIKELY (!tables_decoder.finaliseTable(tables_rev_sym_table))
			{
				return -1;
			}

			SOUP_IF_UNLIKELY (!tables_decoder.readLength(tables_rev_sym_table, literal_syms + offset_syms, kLiteralSyms + kOffsetSyms, code_length, br))
			{
				return -1;
			}
			SOUP_IF_UNLIKELY (!literals_decoder.prepareTable(literals_rev_sym_table, literal_syms, kLiteralSyms, code_length))
			{
				return -1;
			}
			SOUP_IF_UNLIKELY (!offset_decoder.prepareTable(offset_rev_sym_table, offset_syms, kOffsetSyms, code_length + literal_syms))
			{
				return -1;
			}
		}
		else
		{
			unsigned char fixed_literal_code_len[kLiteralSyms];
			unsigned char fixed_offset_code_len[kOffsetSyms];

			for (i = 0; i < 144; i++)
				fixed_literal_code_len[i] = 8;
			for (; i < 256; i++)
				fixed_literal_code_len[i] = 9;
			for (; i < 280; i++)
				fixed_literal_code_len[i] = 7;
			for (; i < kLiteralSyms; i++)
				fixed_literal_code_len[i] = 8;

			for (i = 0; i < kOffsetSyms; i++)
				fixed_offset_code_len[i] = 5;

			SOUP_IF_UNLIKELY (!literals_decoder.prepareTable(literals_rev_sym_table, kLiteralSyms, kLiteralSyms, fixed_literal_code_len))
			{
				return -1;
			}
			SOUP_IF_UNLIKELY (!offset_decoder.prepareTable(offset_rev_sym_table, kOffsetSyms, kOffsetSyms, fixed_offset_code_len))
			{
				return -1;
			}
		}

		for (i = 0; i < kOffsetSyms; i++)
		{
			unsigned int n = offset_rev_sym_table[i];
			if (n < kOffsetSyms)
			{
				offset_rev_sym_table[i] = kOffsetCode[n];
			}
		}

		for (i = 0; i < kLiteralSyms; i++)
		{
			unsigned int n = literals_rev_sym_table[i];
			if (n >= kMatchLenSymStart && n < kLiteralSyms)
			{
				literals_rev_sym_table[i] = kMatchLenCode[n - kMatchLenSymStart];
			}
		}

		SOUP_IF_UNLIKELY (!literals_decoder.finaliseTable(literals_rev_sym_table)
			|| !offset_decoder.finaliseTable(offset_rev_sym_table)
			)
		{
			return -1;
		}

		unsigned char* current_out = out + out_offset;
		const unsigned char* out_end = current_out + block_size_max;
		const unsigned char* out_fast_end = out_end - 15;

		while (true)
		{
			br.refill32();

			unsigned int literals_code_word = literals_decoder.readValue(literals_rev_sym_table, br);
			if (literals_code_word < 256)
			{
				SOUP_IF_UNLIKELY (current_out >= out_end)
				{
					return -1;
				}
				*current_out++ = literals_code_word;
			}
			else
			{
				if (literals_code_word == kEODMarkerSym)
				{
					break;
				}

				SOUP_IF_UNLIKELY (literals_code_word == -1)
				{
					return -1;
				}

				unsigned int match_length = br.getBits((literals_code_word >> 16) & 15);
				SOUP_IF_UNLIKELY (match_length == -1)
				{
					return -1;
				}

				match_length += (literals_code_word & 0x7fff);

				unsigned int offset_code_word = offset_decoder.readValue(offset_rev_sym_table, br);
				SOUP_IF_UNLIKELY (offset_code_word == -1)
				{
					return -1;
				}

				unsigned int match_offset = br.getBits((offset_code_word >> 16) & 15);
				SOUP_IF_UNLIKELY (match_offset == -1)
				{
					return -1;
				}

				match_offset += (offset_code_word & 0x7fff);

				const unsigned char* src = current_out - match_offset;
				SOUP_IF_UNLIKELY (src < out)
				{
					return -1;
				}

				if (match_offset >= 16 && (current_out + match_length) <= out_fast_end)
				{
					const unsigned char* copy_src = src;
					unsigned char* copy_dst = current_out;
					const unsigned char* copy_end_dst = current_out + match_length;

					do
					{
						memcpy(copy_dst, copy_src, 16);
						copy_src += 16;
						copy_dst += 16;
					} while (copy_dst < copy_end_dst);

					current_out += match_length;
				}
				else
				{
					SOUP_IF_UNLIKELY ((current_out + match_length) > out_end)
					{
						return -1;
					}

					while (match_length--)
					{
						*current_out++ = *src++;
					}
				}
			}
		}

		return (unsigned int)(current_out - (out + out_offset));
	}

	bool deflate::gzReadFooter(const void* _data, size_t size, gzFooter& out) noexcept
	{
		// Quick sanity check to make sure this is gzip data
		SOUP_IF_UNLIKELY (size < 18)
		{
			return false;
		}
		const uint8_t* data = static_cast<const uint8_t*>(_data);
		SOUP_IF_UNLIKELY (data[0] != 0x1f || data[1] != 0x8b || data[2] != 0x08)
		{
			return false;
		}

		// Then let's hope it's gzip data all the way through
		MemoryRefReader rr(data + size - 8, 8);
		rr.u32_le(out.uncompressed_data_crc32);
		rr.u32_le(out.uncompressed_size);
		return (out.uncompressed_size >> 7) < size; // And another sanity check; make sure the uncompressed_size does not exceed `size * 128`.
	}

	using DecompressResult = deflate::DecompressResult;

	DecompressResult deflate::gzDecompress(const void* data, size_t size) SOUP_EXCAL
	{
		gzFooter f;
		SOUP_IF_LIKELY (gzReadFooter(data, size, f))
		{
			return decompress(data, size, f.uncompressed_size);
		}
		return {};
	}

	enum class checksum_type : uint8_t
	{
		NONE = 0,
		GZIP,
		ZLIB
	};

	DecompressResult deflate::decompress(const void* compressed_data, size_t compressed_data_size, size_t decompressed_size) SOUP_EXCAL
	{
		DecompressResult res{};
		res.decompressed = std::string(decompressed_size, '\0'); // excal
		res.compressed_size = decompress(compressed_data, compressed_data_size, res.decompressed.data(), decompressed_size, res.checksum_state);
		SOUP_IF_UNLIKELY (res.compressed_size == -1)
		{
			return {};
		}
		res.decompressed.resize(decompressed_size);
		return res;
	}

	size_t deflate::decompress(const void* compressed_data, size_t compressed_data_size, void* _out, size_t& decompressed_size, ChecksumState& out_checksum_state) noexcept
	{
		const size_t max_decompressed_size = decompressed_size;

		const uint8_t* current_compressed_data = static_cast<const uint8_t*>(compressed_data);
		const uint8_t* end_compressed_data = current_compressed_data + compressed_data_size;

		checksum_type checksum_type = checksum_type::NONE;

		SOUP_IF_UNLIKELY ((current_compressed_data + 2) > end_compressed_data)
		{
			return -1;
		}

		if (current_compressed_data[0] == 0x1f && current_compressed_data[1] == 0x8b) // gzip magic
		{
			current_compressed_data += 2;
			if ((current_compressed_data + 8) > end_compressed_data || current_compressed_data[0] != 0x08)
			{
				return -1;
			}

			current_compressed_data++;

			unsigned char flags = *current_compressed_data++;
			current_compressed_data += 6;

			if (flags & 0x02)
			{
				if ((current_compressed_data + 2) > end_compressed_data)
				{
					return -1;
				}

				current_compressed_data += 2;
			}

			if (flags & 0x04)
			{
				if ((current_compressed_data + 2) > end_compressed_data)
				{
					return -1;
				}

				unsigned short extra_field_len = ((unsigned short)current_compressed_data[0]) | (((unsigned short)current_compressed_data[1]) << 8);
				current_compressed_data += 2;

				if ((current_compressed_data + extra_field_len) > end_compressed_data)
				{
					return -1;
				}

				current_compressed_data += extra_field_len;
			}

			if (flags & 0x08)
			{
				do
				{
					if (current_compressed_data >= end_compressed_data)
					{
						return -1;
					}

					current_compressed_data++;
				} while (current_compressed_data[-1]);
			}

			if (flags & 0x10)
			{
				do
				{
					if (current_compressed_data >= end_compressed_data)
					{
						return -1;
					}

					current_compressed_data++;
				} while (current_compressed_data[-1]);
			}

			if (flags & 0x20)
			{
				return -1;
			}

			out_checksum_state = CHKSUM_FAIL;
			checksum_type = checksum_type::GZIP;
		}
		else if ((current_compressed_data[0] & 0x0f) == 0x08)
		{
			unsigned char CMF = current_compressed_data[0];
			unsigned char FLG = current_compressed_data[1];
			unsigned short check = FLG | (((unsigned short)CMF) << 8);

			if ((CMF >> 4) <= 7 && (check % 31) == 0)
			{
				current_compressed_data += 2;
				if (FLG & 0x20)
				{
					if ((current_compressed_data + 4) > end_compressed_data)
					{
						return -1;
					}
					current_compressed_data += 4;
				}
			}

			out_checksum_state = CHKSUM_FAIL;
			checksum_type = checksum_type::ZLIB;
		}

		uint32_t check_sum = crc32::INITIAL;
		if (checksum_type == checksum_type::ZLIB)
		{
			check_sum = adler32::INITIAL;
		}

		DeflateBitReader br(current_compressed_data, end_compressed_data);

		auto out = reinterpret_cast<uint8_t*>(_out);
		size_t current_out_offset = 0;
		bool final_block;
		do
		{
			unsigned int block_result = decompressBlock(reinterpret_cast<Context&>(br), out, current_out_offset, max_decompressed_size, final_block);
			SOUP_IF_UNLIKELY (block_result == -1)
			{
				return -1;
			}

			switch (checksum_type)
			{
			case checksum_type::NONE:
				break;

			case checksum_type::GZIP:
				check_sum = crc32::hash(out + current_out_offset, block_result, check_sum);
				break;

			case checksum_type::ZLIB:
				check_sum = adler32::hash(out + current_out_offset, block_result, check_sum);
				break;
			}

			current_out_offset += block_result;
		} while (!final_block);

		decompressed_size = current_out_offset;

		br.alignToByte();
		current_compressed_data = br.getInBlock();

		unsigned int stored_check_sum;

		switch (checksum_type)
		{
		case checksum_type::NONE:
			break;

		case checksum_type::GZIP:
			if ((current_compressed_data + 4) > end_compressed_data)
			{
				return -1;
			}

			stored_check_sum = ((unsigned int)current_compressed_data[0]);
			stored_check_sum |= ((unsigned int)current_compressed_data[1]) << 8;
			stored_check_sum |= ((unsigned int)current_compressed_data[2]) << 16;
			stored_check_sum |= ((unsigned int)current_compressed_data[3]) << 24;

			if (stored_check_sum == check_sum)
			{
				out_checksum_state = CHKSUM_PASS;
			}

			current_compressed_data += 4;
			break;

		case checksum_type::ZLIB:
			if ((current_compressed_data + 4) > end_compressed_data)
			{
				return -1;
			}

			stored_check_sum = ((unsigned int)current_compressed_data[0]) << 24;
			stored_check_sum |= ((unsigned int)current_compressed_data[1]) << 16;
			stored_check_sum |= ((unsigned int)current_compressed_data[2]) << 8;
			stored_check_sum |= ((unsigned int)current_compressed_data[3]);

			if (stored_check_sum == check_sum)
			{
				out_checksum_state = CHKSUM_PASS;
			}

			current_compressed_data += 4;
			break;
		}

		return (current_compressed_data - (unsigned char*)compressed_data);
	}

	void deflate::initContext(Context& ctx, const uint8_t* compressed_data, size_t compressed_data_size) noexcept
	{
		soup::construct_at<DeflateBitReader>(reinterpret_cast<DeflateBitReader*>(&ctx), compressed_data, compressed_data + compressed_data_size);
	}

	unsigned int deflate::decompressBlock(Context& ctx, uint8_t* out, size_t current_out_offset, size_t max_decompressed_size, bool& final_block) noexcept
	{
		auto& br = reinterpret_cast<DeflateBitReader&>(ctx);
		final_block = br.getBits(1);
		const auto block_type = br.getBits(2);
		switch (block_type)
		{
		case 0:
			return copy_stored(br, out, current_out_offset, max_decompressed_size - current_out_offset);

		case 1:
			return decompress_block(br, false, out, current_out_offset, max_decompressed_size - current_out_offset);

		case 2:
			return decompress_block(br, true, out, current_out_offset, max_decompressed_size - current_out_offset);

		case 3:
			return -1;
		}
		SOUP_UNREACHABLE;
	}

	std::string deflate::decompressZeroTerminated(const std::string& data) SOUP_EXCAL
	{
		Context ctx;
		initContext(ctx, reinterpret_cast<const uint8_t*>(data.data()), data.size());
		const size_t max_decompressed_size = getMaxDecompressedSize(data.data(), data.size());
		std::string out(max_decompressed_size, '\0'); // excal
		size_t current_out_offset = 0;
		unsigned int block_result;
		do {
			bool final_block;
			block_result = decompressBlock(ctx, reinterpret_cast<uint8_t*>(out.data()), current_out_offset, max_decompressed_size, final_block);
			SOUP_IF_UNLIKELY (block_result == -1)
			{
				return {};
			}
			current_out_offset += block_result;
		} while (block_result != 0);
		out.resize(current_out_offset);
		return out;
	}

	std::string deflate::compress(const void* data, size_t size, uint_fast8_t level, WrapperType wt)
	{
		auto compressed = compressRaw(data, size, level, wt);
		if (compressed.size() > encodeUncompressed_size(size, wt))
		{
			// Well, that was a waste.
			compressed = encodeUncompressed(data, size, wt);
		}
		return compressed;
	}

	std::string deflate::compressRaw(const void* data, size_t size, uint_fast8_t level, WrapperType wt)
	{
		const std::string encoded = Lz77Compressor::compress(reinterpret_cast<const uint8_t*>(data), size, level);

		StringWriter sw;
		if (wt == WT_ZLIB)
		{
			writeZlibHeader(sw);
		}
		else if (wt == WT_GZIP)
		{
			writeGzipHeader(sw);
		}
		encodeDynamic(sw, encoded.data(), encoded.size(), wt != WT_DEFLATE_STREAM);
		if (wt == WT_ZLIB)
		{
			writeZlibFooter(sw, data, size);
		}
		else if (wt == WT_GZIP)
		{
			writeGzipFooter(sw, data, size);
		}

		// For small inputs, static huffman coding might be worth it
		if (encoded.size() < 100)
		{
			StringWriter static_sw;
			if (wt == WT_ZLIB)
			{
				writeZlibHeader(static_sw);
			}
			else if (wt == WT_GZIP)
			{
				writeGzipHeader(static_sw);
			}
			encodeStatic(static_sw, encoded.data(), encoded.size(), wt != WT_DEFLATE_STREAM);
			if (wt == WT_ZLIB)
			{
				writeZlibFooter(static_sw, data, size);
			}
			else if (wt == WT_GZIP)
			{
				writeGzipFooter(static_sw, data, size);
			}

			if (sw.data.size() > static_sw.data.size())
			{
				SOUP_MOVE_RETURN(static_sw.data);
			}
		}

		SOUP_MOVE_RETURN(sw.data);
	}

	std::string deflate::compressHuffman(const uint8_t* data, size_t size, WrapperType wt)
	{
		size_t frequencies[257];
		memset(frequencies, 0, sizeof(frequencies));
		for (size_t i = 0; i != size; ++i)
		{
			++frequencies[data[i]];
		}
		frequencies[256] = 1;

		// We don't use distance codes, but at least 1 is required
		uint8_t lengths[257 + 1];
		lengths[257] = 0;

		// Build the literals/lengths tree
		SOUP_ASSERT(huffman_lengths(lengths, frequencies, 257) <= 15);
		uint16_t codes[257];
		huffman_codes(codes, lengths, 257);
		/*for (uint_fast16_t i = 0; i != 257; ++i)
		{
			std::cout << (int)i << ": " << string::binary(codes[i]) << ", " << (int)lengths[i] << "\n";
		}*/

		StringWriter sw;
		if (wt == WT_ZLIB)
		{
			writeZlibHeader(sw);
		}
		else if (wt == WT_GZIP)
		{
			writeGzipHeader(sw);
		}

		BitWriter bw(&sw);
		writeDynamicHuffmanHeader(bw, lengths, 257, 1, wt != WT_DEFLATE_STREAM);
		for (size_t i = 0; i != size; ++i)
		{
			const uint8_t value = data[i];
			bw.t(lengths[value], codes[value]);
		}
		bw.t(lengths[256], codes[256]);
		bw.finishByte();

		if (wt == WT_ZLIB)
		{
			writeZlibFooter(sw, data, size);
		}
		else if (wt == WT_GZIP)
		{
			writeGzipFooter(sw, data, size);
		}

		SOUP_MOVE_RETURN(sw.data);
	}

	void deflate::writeZlibHeader(StringWriter& sw)
	{
		sw.data.push_back(0x78);
		sw.data.push_back(0x01);
	}

	void deflate::writeGzipHeader(StringWriter& sw)
	{
		sw.data.push_back(0x1f);
		sw.data.push_back(0x8bu);
		sw.data.push_back(0x08);
		sw.data.push_back(0x00);
		sw.data.push_back(0x00);
		sw.data.push_back(0x00);
		sw.data.push_back(0x00);
		sw.data.push_back(0x00);
		sw.data.push_back(0x00);
		sw.data.push_back(0x03);
	}

	void deflate::writeZlibFooter(Writer& w, const void* uncompressed_data, size_t uncompressed_size)
	{
		uint32_t hash = adler32::hash(uncompressed_data, uncompressed_size);
		w.u32_be(hash);
	}

	void deflate::writeGzipFooter(Writer& w, const void* uncompressed_data, size_t uncompressed_size)
	{
		uint32_t dw = crc32::hash(reinterpret_cast<const uint8_t*>(uncompressed_data), uncompressed_size);
		w.u32_le(dw);
		dw = static_cast<uint32_t>(uncompressed_size);
		w.u32_le(dw);
	}

	void deflate::encodeDynamic(Writer& w, const void* data, size_t size, bool bfinal)
	{
		size_t litlen_weights[286];
		memset(litlen_weights, 0, sizeof(litlen_weights));
		size_t dist_weights[32];
		memset(dist_weights, 0, sizeof(dist_weights));
		uint_fast16_t litlen_highest_code = 256;
		uint_fast16_t dist_highest_code = 0;
		for (MemoryRefReader r(data, size); r.hasMore(); )
		{
			uint64_t v = 0;
			r.u64_dyn_bp(v);
			const uint_fast16_t code = v & 0x1ff;
			SOUP_ASSERT(code < 286);
			++litlen_weights[code];
			if (code > litlen_highest_code)
			{
				litlen_highest_code = code;
			}
			if (code >= kMatchLenSymStart)
			{
				const uint8_t dist_code = (v >> 14) & 0b11111;
				++dist_weights[dist_code];
				if (dist_code > dist_highest_code)
				{
					dist_highest_code = dist_code;
				}
			}
		}
		litlen_weights[256] = 1;
		const uint_fast16_t litlen_size = litlen_highest_code + 1;
		const uint_fast16_t dist_size = dist_highest_code + 1;

		uint8_t lengths[286 + 32];
		uint16_t codes[286 + 32];

		uint8_t* const litlen_lengths = &lengths[0];
		SOUP_ASSERT(huffman_lengths(litlen_lengths, litlen_weights, litlen_size) <= 15);
		uint16_t* const litlen_codes = &codes[0];
		huffman_codes(litlen_codes, litlen_lengths, litlen_size);

		uint8_t* const dist_lengths = &lengths[litlen_size];
		SOUP_ASSERT(huffman_lengths(dist_lengths, dist_weights, dist_size) <= 15);
		uint16_t* const dist_codes = &codes[litlen_size];
		huffman_codes(dist_codes, dist_lengths, dist_size);

		BitWriter bw(&w);
		writeDynamicHuffmanHeader(bw, lengths, litlen_size, dist_size, bfinal);
		writeHuffmanBody(bw, data, size, litlen_lengths, litlen_codes, dist_lengths, dist_codes);
		bw.finishByte();
	}

	void deflate::encodeStatic(Writer& w, const void* data, size_t size, bool bfinal)
	{
		uint8_t litlen_lengths[288];
		uint16_t litlen_codes[288];
		for (uint_fast16_t i = 0; i != 288; ++i)
		{
			uint16_t code;
			uint8_t len;
			if (i < 144)
			{
				len = 8;
				code = 0x30 + i;
			}
			else if (i < 256)
			{
				len = 9;
				code = 0x190 + (i - 144);
			}
			else if (i < 280)
			{
				len = 7;
				code = i - 256;
			}
			else
			{
				len = 8;
				code = 0xC0 + (i - 280);
			}
			litlen_lengths[i] = len;
			litlen_codes[i] = bitutil::reverse(code, len);
		}

		uint8_t dist_lengths[32];
		uint16_t dist_codes[32];
		for (uint_fast16_t i = 0; i != 32; ++i)
		{
			dist_lengths[i] = 5;
			dist_codes[i] = bitutil::reverse(i, 5);
		}

		BitWriter bw(&w);
		bw.b(bfinal); // BFINAL
		bw.t(2, 1); // BTYPE = 01 (static huffman)
		writeHuffmanBody(bw, data, size, litlen_lengths, litlen_codes, dist_lengths, dist_codes);
		bw.finishByte();
	}

	uint32_t deflate::encodeLengthDistancePair(uint_fast16_t length, uint_fast16_t distance) noexcept
	{
		uint32_t out = 0;
		// 9 bits: length symbol
		// 5 bits: length extra
		// 5 bits: distance symbol
		// 13 bits: distance extras

		// Find appropriate length symbol
		for (uint_fast16_t i = 0; i != kMatchLenSyms; ++i)
		{
			const uint_fast16_t base = kMatchLenCode[i] & 0x7fff;
			const uint_fast16_t bits = (kMatchLenCode[i] >> 16) & 15;
			if (base <= length
				&& (length - base) >> bits == 0
				&& (length != 258 || i == kMatchLenSyms - 1)
				)
			{
				out = (kMatchLenSymStart + i);
				out |= ((length - base) & 0b11111) << 9;
				break;
			}
		}

		SOUP_IF_LIKELY (out) // Found a length symbol?
		{
			// Find appropriate distance symbol
			for (uint_fast16_t i = 0; i != kOffsetSyms; ++i)
			{
				const uint_fast16_t base = kOffsetCode[i] & 0x7fff;
				const uint_fast16_t bits = (kOffsetCode[i] >> 16) & 15;
				if (base <= distance
					&& (distance - base) >> bits == 0
					)
				{
					out |= static_cast<uint32_t>(i) << 14;
					out |= static_cast<uint32_t>(distance - base) << 19;
					break;
				}
			}
		}
		return out;
	}

	void deflate::writeDynamicHuffmanHeader(BitWriter& bw, uint8_t lengths[], uint_fast16_t litlen_size, uint_fast16_t dist_size, bool bfinal)
	{
		bw.b(bfinal); // BFINAL
		bw.u8(2, 2); // BTYPE = 10 (dynamic huffman)
		bw.u8(5, static_cast<uint8_t>(litlen_size - 257)); // HLIT (257..286)
		bw.u8(5, static_cast<uint8_t>(dist_size - 1)); // HDIST (1..32)

		const uint_fast16_t size = litlen_size + dist_size;

		// RLE the combined `lengths`
		struct RleSym { uint8_t sym; uint8_t extra; };
		std::vector<RleSym> rle;
		for (uint_fast16_t i = 0; i != size; )
		{
			const uint8_t len = lengths[i];
			uint_fast16_t run = 1;
			while (i + run != size && lengths[i + run] == len)
			{
				++run;
			}
			i += run;

			if (len == 0)
			{
				while (run >= 11)
				{
					const uint_fast16_t n = (run > 138) ? 138 : run;
					rle.push_back({ 18, (uint8_t)(n - 11) });
					run -= n;
				}
				if (run >= 3)
				{
					rle.push_back({ 17, (uint8_t)(run - 3) });
					run = 0;
				}
				while (run--)
				{
					rle.push_back({ 0, 0 });
				}
			}
			else
			{
				rle.push_back({ len, 0 });
				--run;
				while (run >= 3)
				{
					const uint_fast16_t n = (run > 6) ? 6 : run;
					rle.push_back({ 16, (uint8_t)(n - 3) });
					run -= n;
				}
				while (run--)
				{
					rle.push_back({ len, 0 });
				}
			}
		}

		// Huffman code for the code-length alphabet (19 symbols, max 7 bits)
		size_t rle_freq[19];
		memset(rle_freq, 0, sizeof(rle_freq));
		for (const auto& r : rle)
		{
			//std::cout << "rle " << (int)r.sym << ", " << (int)r.extra << "\n";
			++rle_freq[r.sym];
		}
		uint8_t rle_lengths[19];
		SOUP_ASSERT(huffman_lengths(rle_lengths, rle_freq, 19) <= 7);
		uint16_t rle_codes[19];
		huffman_codes(rle_codes, rle_lengths, 19);

		uint8_t hclen = 19;
		while (hclen > 4 && rle_lengths[code_len_syms[hclen - 1]] == 0)
		{
			--hclen;
		}
		//std::cout << "hclen = " << (int)hclen << "\n";

		bw.u8(4, hclen - 4);
		for (size_t i = 0; i != hclen; ++i)
		{
			//std::cout << "op " << (int)code_len_syms[i] << " length " << (int)rle_lengths[code_len_syms[i]] << " (code " << rle_codes[code_len_syms[i]] << ")\n";
			bw.u8(3, rle_lengths[code_len_syms[i]]);
		}

		// The RLE'd code lengths, followed by extra bits where applicable
		for (const auto& r : rle)
		{
			bw.t(rle_lengths[r.sym], rle_codes[r.sym]);
			if (r.sym == 16)
			{
				bw.u8(2, r.extra);
			}
			else if (r.sym == 17)
			{
				bw.u8(3, r.extra);
			}
			else if (r.sym == 18)
			{
				bw.u8(7, r.extra);
			}
		}
	}

	void deflate::writeHuffmanBody(BitWriter& bw, const void* data, size_t size, const uint8_t litlen_lengths[], const uint16_t litlen_codes[], const uint8_t dist_lengths[], const uint16_t dist_codes[])
	{
		for (MemoryRefReader r(data, size); r.hasMore(); )
		{
			uint64_t v = 0;
			r.u64_dyn_bp(v);
			const uint_fast16_t code = v & 0x1ff;
			bw.t(litlen_lengths[code], litlen_codes[code]);
			if (code >= kMatchLenSymStart)
			{
				const uint_fast16_t bits = (kMatchLenCode[code - kMatchLenSymStart] >> 16) & 15;
				bw.u8(bits, (v >> 9) & 0b11111);

				const uint8_t dist_code = (v >> 14) & 0b11111;
				bw.t(dist_lengths[dist_code], dist_codes[dist_code]);

				const uint_fast16_t dist_bits = (kOffsetCode[dist_code] >> 16) & 15;
				bw.t<uint16_t>(dist_bits, static_cast<uint16_t>(v >> 19));
			}
		}
		bw.t(litlen_lengths[256], litlen_codes[256]);
	}

	std::string deflate::encodeUncompressed(const void* data, size_t size, WrapperType wt) SOUP_EXCAL
	{
		std::string out(encodeUncompressed_size(size, wt), '\0');
		encodeUncompressed(reinterpret_cast<const char*>(data), size, out.data(), wt);
		return out;
	}

	void deflate::encodeUncompressed(const char* const data, const size_t size, char out[], WrapperType wt) noexcept
	{
		if (wt == WT_ZLIB)
		{
			*out++ = 0x78;
			*out++ = 0x01;
		}
		else if (wt == WT_GZIP)
		{
			*out++ = 0x1f;
			*out++ = 0x8bu;
			*out++ = 0x08;
			*out++ = 0x00;
			*out++ = 0x00;
			*out++ = 0x00;
			*out++ = 0x00;
			*out++ = 0x00;
			*out++ = 0x00;
			*out++ = 0x03;
		}
		const char* block_data = data;
		size_t block_size = size;
		for (; block_size > 0xffff; block_data += 0xffff, block_size -= 0xffff)
		{
			*out++ = 0; // BFINAL = 0, BTYPE = 0
			*reinterpret_cast<uint16_t*>(out) = 0xffff; out += 2;
			*reinterpret_cast<uint16_t*>(out) = 0x0000 /* ~0xffff */; out += 2;
			memcpy(out, block_data, 0xffff); out += 0xffff;
		}
		*out++ = (wt != WT_DEFLATE_STREAM) ? 1 : 0; // BFINAL = 1*, BTYPE = 0
		*out++ = block_size & 0xff;
		*out++ = (block_size >> 8) & 0xff;
		uint16_t isize = static_cast<uint16_t>(~block_size);
		*out++ = isize & 0xff;
		*out++ = (isize >> 8) & 0xff;
		memcpy(out, block_data, block_size); out += block_size;
		if (wt == WT_ZLIB)
		{
			*reinterpret_cast<uint32_t*>(out) = Endianness::toNetwork(adler32::hash(data, size)); out += 4;
		}
		else if (wt == WT_GZIP)
		{
			if constexpr (ENDIAN_NATIVE == ENDIAN_LITTLE)
			{
				*reinterpret_cast<uint32_t*>(out) = crc32::hash(reinterpret_cast<const uint8_t*>(data), size); out += 4;
				*reinterpret_cast<uint32_t*>(out) = static_cast<uint32_t>(size); out += 4;
			}
			else
			{
				*reinterpret_cast<uint32_t*>(out) = Endianness::invert(crc32::hash(reinterpret_cast<const uint8_t*>(data), size)); out += 4;
				*reinterpret_cast<uint32_t*>(out) = Endianness::invert(static_cast<uint32_t>(size)); out += 4;
			}
		}
	}
}
