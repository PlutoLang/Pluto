#include "unicode.hpp"

#include "bitutil.hpp"

NAMESPACE_SOUP
{
	char32_t unicode::utf8_to_utf32_char(std::string::const_iterator& it, const std::string::const_iterator end) noexcept
	{
		uint8_t ch = *it++;
		if (!UTF8_HAS_CONTINUATION(ch))
		{
			return ch;
		}
		SOUP_IF_UNLIKELY (UTF8_IS_CONTINUATION(ch))
		{
			return REPLACEMENT_CHAR;
		}

		// 11110xxx: todo = 3
		// 1110xxxx: todo = 2
		// 110xxxxx: todo = 1
		uint8_t todo = bitutil::getNumLeadingZeros(static_cast<uint32_t>((uint8_t)~ch)) - 25;

		// Copy 'x' from above into 'uni'
		uint32_t uni = ch & ((1 << (6 - todo)) - 1);

		for (uint8_t j = 0; j != todo; ++j)
		{
			SOUP_IF_UNLIKELY (it == end)
			{
				return REPLACEMENT_CHAR;
			}
			uint8_t ch = *it++;
			SOUP_IF_UNLIKELY (!UTF8_IS_CONTINUATION(ch))
			{
				--it;
				return REPLACEMENT_CHAR;
			}
			uni <<= 6;
			uni |= (ch & 0b111111);
		}
		/*SOUP_IF_UNLIKELY ((uni >= 0xD800 && uni <= 0xDFFF)
			|| uni > 0x10FFFF
			)
		{
			return REPLACEMENT_CHAR;
		}*/
		return uni;
	}

#if SOUP_CPP20
	std::u32string unicode::utf8_to_utf32(const char8_t* utf8) noexcept
	{
		return utf8_to_utf32(reinterpret_cast<const char*>(utf8));
	}
#endif

	std::u32string unicode::utf8_to_utf32(const std::string& utf8) noexcept
	{
		std::u32string utf32{};
		utf32.reserve(utf8_char_len(utf8));
		auto it = utf8.cbegin();
		const auto end = utf8.cend();
		while (it != end)
		{
			utf32.push_back(utf8_to_utf32_char(it, end));
		}
		return utf32;
	}

#if SOUP_CPP20
	UTF16_STRING_TYPE unicode::utf8_to_utf16(const char8_t* utf8) noexcept
	{
		return utf8_to_utf16(reinterpret_cast<const char*>(utf8));
	}
#endif

	UTF16_STRING_TYPE unicode::utf8_to_utf16(const std::string& utf8) noexcept
	{
#if SOUP_WINDOWS
		std::wstring utf16;
		const int sizeRequired = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
		SOUP_IF_LIKELY (sizeRequired != 0)
		{
			utf16 = std::wstring(sizeRequired, 0);
			MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), utf16.data(), sizeRequired);
		}
		return utf16;
#else
		UTF16_STRING_TYPE utf16{};
		utf16.reserve(utf8.size()); // Note: we could end up with a slightly oversized buffer here if UTF8 input has many 3 or 4 byte symbols
		auto it = utf8.cbegin();
		const auto end = utf8.cend();
		while (it != end)
		{
			utf32_to_utf16_char(utf16, utf8_to_utf32_char(it, end));
		}
		return utf16;
#endif
	}

#if SOUP_WINDOWS
	UTF16_STRING_TYPE unicode::acp_to_utf16(const std::string& acp) noexcept
	{
		std::wstring utf16;
		const int sizeRequired = MultiByteToWideChar(CP_ACP, 0, acp.data(), (int)acp.size(), nullptr, 0);
		SOUP_IF_LIKELY (sizeRequired != 0)
		{
			utf16 = std::wstring(sizeRequired, 0);
			MultiByteToWideChar(CP_ACP, 0, acp.data(), (int)acp.size(), utf16.data(), sizeRequired);
		}
		return utf16;
	}
#endif

	UTF16_STRING_TYPE unicode::utf32_to_utf16(const std::u32string& utf32) noexcept
	{
		UTF16_STRING_TYPE utf16{};
		utf16.reserve(utf32.size());
		for (char32_t c : utf32)
		{
			utf32_to_utf16_char(utf16, c);
		}
		return utf16;
	}

	void unicode::utf32_to_utf16_char(UTF16_STRING_TYPE& utf16, char32_t c) noexcept
	{
		if (c <= 0xFFFF)
		{
			utf16.push_back((UTF16_CHAR_TYPE)c);
		}
		else
		{
			c -= 0x10000;
			utf16.push_back((UTF16_CHAR_TYPE)((c >> 10) + 0xD800));
			utf16.push_back((UTF16_CHAR_TYPE)((c & 0x3FF) + 0xDC00));
		}
	}

	std::string unicode::utf32_to_utf8(char32_t utf32) noexcept
	{
		// 1
		if (utf32 < 0b10000000)
		{
			return std::string(1, (char)utf32);
		}
		// 2
		std::string utf8(1, (char)((utf32 & 0b111111) | UTF8_CONTINUATION_FLAG));
		utf32 >>= 6;
		if (utf32 <= 0b11111)
		{
			utf8.insert(0, 1, (char)(utf32 | 0b11000000)); // 110xxxxx
			return utf8;
		}
		// 3
		utf8.insert(0, 1, (char)((utf32 & 0b111111) | UTF8_CONTINUATION_FLAG));
		utf32 >>= 6;
		if (utf32 <= 0b1111)
		{
			utf8.insert(0, 1, (char)(utf32 | 0b11100000)); // 1110xxxx
			return utf8;
		}
		// 4
		utf8.insert(0, 1, (char)((utf32 & 0b111111) | UTF8_CONTINUATION_FLAG));
		utf32 >>= 6;
		utf8.insert(0, 1, (char)(utf32 | 0b11110000)); // 11110xxx
		return utf8;
	}

	std::string unicode::utf32_to_utf8(const std::u32string& utf32) noexcept
	{
		std::string utf8{};
		utf8.reserve(utf32.size());
		for (const char32_t& c : utf32)
		{
			utf8.append(utf32_to_utf8(c));
		}
		return utf8;
	}

	size_t unicode::utf8_char_len(const std::string& str) noexcept
	{
		size_t char_len = 0;
		for (size_t i = 0; i != str.size(); ++i)
		{
			char_len += !UTF8_IS_CONTINUATION(str[i]);
		}
		return char_len;
	}

	size_t unicode::utf16_char_len(const UTF16_STRING_TYPE& str) noexcept
	{
		size_t char_len = 0;
		for (size_t i = 0; i != str.size(); ++i)
		{
			char_len += !UTF16_IS_LOW_SURROGATE(str[i]);
		}
		return char_len;
	}
}
