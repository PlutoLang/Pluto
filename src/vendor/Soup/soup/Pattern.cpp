#include "Pattern.hpp"

#include "CompiletimePatternWithOptBytesBase.hpp"
#include "Module.hpp"
#include "Pointer.hpp"
#include "Reader.hpp"
#include "string.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	Pattern::Pattern(const CompiletimePatternWithOptBytesBase& sig)
		: bytes(sig.getVec())
	{
	}

	Pattern::Pattern(const std::string& str)
		: Pattern(str.data(), str.size())
	{
	}

	Pattern::Pattern(const char* str, size_t len)
	{
		auto to_hex = [&](char c) -> std::optional<std::uint8_t>
		{
			switch (string::upper_char(c))
			{
			case '0':
				return static_cast<std::uint8_t>(0);
			case '1':
				return static_cast<std::uint8_t>(1);
			case '2':
				return static_cast<std::uint8_t>(2);
			case '3':
				return static_cast<std::uint8_t>(3);
			case '4':
				return static_cast<std::uint8_t>(4);
			case '5':
				return static_cast<std::uint8_t>(5);
			case '6':
				return static_cast<std::uint8_t>(6);
			case '7':
				return static_cast<std::uint8_t>(7);
			case '8':
				return static_cast<std::uint8_t>(8);
			case '9':
				return static_cast<std::uint8_t>(9);
			case 'A':
				return static_cast<std::uint8_t>(10);
			case 'B':
				return static_cast<std::uint8_t>(11);
			case 'C':
				return static_cast<std::uint8_t>(12);
			case 'D':
				return static_cast<std::uint8_t>(13);
			case 'E':
				return static_cast<std::uint8_t>(14);
			case 'F':
				return static_cast<std::uint8_t>(15);
			default:
				return std::nullopt;
			}
		};

		for (size_t i = 0; i < len; ++i)
		{
			if (str[i] == ' ')
			{
				continue;
			}

			const bool last = (i == len - 1);
			if (str[i] != '?')
			{
				if (!last)
				{
					auto c1 = to_hex(str[i]);
					auto c2 = to_hex(str[i + 1]);

					if (c1 && c2)
					{
						bytes.emplace_back(static_cast<std::uint8_t>((*c1 * 0x10) + *c2));
					}
				}
			}
			else
			{
				bytes.emplace_back(std::nullopt);
			}
		}
	}

	Pattern::Pattern(const char* bin, const char* mask)
	{
		const size_t len = strlen(mask);
		bytes.reserve(len);
		for (size_t i = 0; i != len; ++i)
		{
			if (mask[i] == '?')
			{
				bytes.emplace_back(std::nullopt);
			}
			else // if (mask[i] == 'x')
			{
				bytes.emplace_back(reinterpret_cast<const uint8_t*>(bin)[i]);
			}
		}
	}

	bool Pattern::matches(uint8_t* target) const noexcept
	{
#if SOUP_WINDOWS && !SOUP_CROSS_COMPILE
		__try
		{
#endif
			for (size_t i = 0; i != bytes.size(); ++i)
			{
				if (bytes[i] && *bytes[i] != target[i])
				{
					return false;
				}
			}
			return true;
#if SOUP_WINDOWS && !SOUP_CROSS_COMPILE
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return false;
#endif
	}

	std::string Pattern::toString() const SOUP_EXCAL
	{
		std::string str;
		for (const auto& b : bytes)
		{
			if (b.has_value())
			{
				str.append(string::lpad(string::hex(b.value()), 2, '0'));
			}
			else
			{
				str.push_back('?');
			}
			str.push_back(' ');
		}
		if (!str.empty())
		{
			str.pop_back();
		}
		return str;
	}

#if SOUP_X86 && SOUP_BITS == 64
	/* Generated using memHistogram
	uint8_t frequencies[256];
	h.toFrequencies(frequencies);
	for (uint_fast16_t i = 0; i != 256; ++i)
	{
		std::cout << "0x" << string::bin2hex((const char*)&frequencies[i], 1) << ",";
		if (((i + 1) % 16) == 0)
		{
			std::cout << "\n";
		}
	}
	*/
	static const uint8_t frequencies[256]
	{
		0xFF,0xFA,0xF1,0xE4,0xDC,0xEA,0xB8,0xBD,0xED,0x97,0x99,0xA3,0xC8,0xD4,0x9E,0xFB,
		0xEF,0xC9,0x6F,0x91,0x96,0xD0,0xA7,0x5E,0xC7,0x4A,0x2D,0x64,0x85,0x53,0x58,0x7B,
		0xEB,0x8F,0x28,0x5F,0xF9,0x40,0x1C,0x47,0xE5,0x98,0x31,0x81,0x63,0x26,0x24,0x78,
		0xDE,0x13,0x4C,0xCF,0x74,0x33,0x0C,0x07,0xD2,0x79,0x3E,0xCE,0x90,0x77,0x6D,0x93,
		0xE3,0xF5,0x95,0xB0,0xF2,0xE6,0x86,0x88,0xFE,0xE9,0xB9,0xB2,0xF3,0xD3,0x5C,0x67,
		0xDF,0x69,0x6E,0x92,0xBE,0xAB,0x7A,0xB1,0xCD,0xB4,0x11,0x7F,0xD5,0x83,0x6B,0xAE,
		0xBF,0xAA,0x22,0x80,0x9C,0xA6,0xC3,0x2C,0xA9,0x8D,0x08,0x34,0xA0,0x68,0x7E,0x87,
		0xCC,0x05,0x8C,0x9B,0xEE,0xDB,0x4E,0x36,0xA5,0x37,0x02,0x3A,0x94,0x3F,0x29,0xE7,
		0xDD,0xA8,0x3D,0xF0,0xDA,0xE2,0x56,0x8E,0xC5,0xF8,0x71,0xFD,0x59,0xF7,0x35,0x43,
		0xD8,0x1F,0x10,0x1D,0x32,0x20,0x84,0x21,0x7D,0x15,0x03,0x00,0x42,0x09,0x04,0x0D,
		0xAF,0x46,0x3B,0x51,0x5D,0x4B,0x44,0x57,0x9A,0x48,0x82,0x5A,0x8A,0x41,0x5B,0x61,
		0xBA,0x0B,0x1A,0x16,0x38,0x06,0x9D,0x6C,0xBB,0x66,0x72,0x3C,0x49,0x2B,0x54,0x50,
		0xEC,0xD9,0xA2,0xE0,0xD7,0x8B,0xC6,0xC4,0xD6,0xC2,0x76,0xC1,0xF4,0x7C,0xA4,0xB6,
		0xD1,0xA1,0x9F,0x65,0x27,0x19,0x4F,0x62,0xAD,0x73,0x39,0x60,0x1E,0x0F,0x17,0x2E,
		0xB7,0x4D,0x23,0x25,0x55,0x01,0x0E,0x12,0xF6,0xCA,0x2F,0xBC,0xB3,0x1B,0x0A,0x18,
		0xC0,0x45,0x75,0xE8,0x30,0x14,0xE1,0xCB,0xB5,0x70,0x6A,0x2A,0x52,0x89,0xAC,0xFC,
	};

	size_t Pattern::getMostUniqueByteIndex() const noexcept
	{
		size_t best_index = 0;
		uint8_t best_score = 0xFF;
		for (size_t i = 0; i != bytes.size(); ++i)
		{
			if (bytes[i].has_value())
			{
				const uint8_t f = frequencies[bytes[i].value()];
				if (best_score > f)
				{
					best_index = i;
					best_score = f;
				}
			}
		}
		return best_index;
	}
#endif

	bool Pattern::hasWildcards() const noexcept
	{
		for (const auto& o : bytes)
		{
			if (!o.has_value())
			{
				return true;
			}
		}
		return false;
	}

	bool Pattern::io(Writer& w) noexcept
	{
		{
			uint64_t size = bytes.size();
			SOUP_RETHROW_FALSE(w.u64_dyn_bp(size));
		}

		uint8_t none_value = 0;
		if (hasWildcards())
		{
		_retry:
			for (const auto& o : bytes)
			{
				if (o.has_value() && static_cast<int64_t>(*o) == none_value)
				{
					SOUP_RETHROW_FALSE(none_value != 0xff); // Possible future expansion, but right now none_value must fit in a byte and must not be used in the pattern.
					++none_value;
					goto _retry;
				}
			}
			SOUP_RETHROW_FALSE(w.i64_dyn_bp(static_cast<int64_t>(static_cast<uint64_t>(none_value))));
		}
		else
		{
			SOUP_RETHROW_FALSE(w.i64_dyn_bp(-1));
		}

		for (const auto& o : bytes)
		{
			uint8_t v = o.value_or(none_value);
			SOUP_RETHROW_FALSE(w.u8(v));
		}

		return true;
	}

	bool Pattern::io(Reader& r) SOUP_EXCAL
	{
		bytes.clear();

		uint64_t size;
		SOUP_RETHROW_FALSE(r.u64_dyn_bp(size));
		bytes.reserve(size);

		int64_t none_value;
		SOUP_RETHROW_FALSE(r.i64_dyn_bp(none_value));
		SOUP_RETHROW_FALSE(none_value >= -1 && none_value <= 0xff);

		while (size--)
		{
			uint8_t v;
			r.u8(v);
			if (static_cast<int64_t>(v) != none_value)
			{
				bytes.emplace_back(v);
			}
			else
			{
				bytes.emplace_back(std::nullopt);
			}
		}

		return true;
	}
}
