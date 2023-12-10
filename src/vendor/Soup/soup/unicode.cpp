#include "unicode.hpp"

namespace soup
{
	char32_t unicode::utf8_to_utf32_char(std::string::const_iterator& it, const std::string::const_iterator end) noexcept
	{
		uint32_t uni;
		uint8_t todo = 0;
		uint8_t ch = *it++;
		if (UTF8_HAS_CONTINUATION(ch))
		{
			if (UTF8_IS_CONTINUATION(ch))
			{
				return 0;
			}
			if ((ch & 0b01111000) == 0b01110000) // 11110xxx
			{
				uni = (ch & 0b111);
				todo = 3;
			}
			else if ((ch & 0b01110000) == 0b01100000) // 1110xxxx
			{
				uni = (ch & 0b1111);
				todo = 2;
			}
			else //if ((ch & 0b01100000) == 0b01000000) // 110xxxxx
			{
				uni = (ch & 0b11111);
				todo = 1;
			}
		}
		else // 0xxxxxxx
		{
			uni = ch;
		}
		for (uint8_t j = 0; j < todo; ++j)
		{
			if (it == end)
			{
				return 0;
			}
			uint8_t ch = *it++;
			if (!UTF8_IS_CONTINUATION(ch))
			{
				break;
			}
			uni <<= 6;
			uni += (ch & 0b111111);
		}
		if ((uni >= 0xD800 && uni <= 0xDFFF)
			|| uni > 0x10FFFF
			)
		{
			return 0;
		}
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
		auto it = utf8.cbegin();
		const auto end = utf8.cend();
		while (it != end)
		{
			auto uni = utf8_to_utf32_char(it, end);
			if (uni == 0)
			{
				utf32.push_back(REPLACEMENT_CHAR);
			}
			else
			{
				utf32.push_back(uni);
			}
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
		return utf32_to_utf16(utf8_to_utf32(utf8));
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
		for (char32_t c : utf32)
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
		return utf16;
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
			utf8.insert(0, 1, (char)((utf32 & 0b11111) | 0b11000000)); // 110xxxxx
			return utf8;
		}
		// 3
		utf8.insert(0, 1, (char)((utf32 & 0b111111) | UTF8_CONTINUATION_FLAG));
		utf32 >>= 6;
		if (utf32 <= 0b1111)
		{
			utf8.insert(0, 1, (char)((utf32 & 0b1111) | 0b11100000)); // 1110xxxx
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
		for (const char32_t& c : utf32)
		{
			utf8.append(utf32_to_utf8(c));
		}
		return utf8;
	}

	size_t unicode::utf8_char_len(const std::string& str) noexcept
	{
		size_t char_len = 0;
		for (const auto& c : str)
		{
			if (!UTF8_IS_CONTINUATION(c))
			{
				++char_len;
			}
		}
		return char_len;
	}

	size_t unicode::utf16_char_len(const UTF16_STRING_TYPE& str) noexcept
	{
		size_t char_len = 0;
		for (const auto& c : str)
		{
			SOUP_IF_LIKELY (!UTF16_IS_LOW_SURROGATE(c))
			{
				++char_len;
			}
		}
		return char_len;
	}
}
