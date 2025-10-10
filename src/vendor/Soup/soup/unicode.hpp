#pragma once

#include <cstdint>
#include <string>

#include "base.hpp"

#define UTF8_CONTINUATION_FLAG 0b10000000
#define UTF8_HAS_CONTINUATION(ch) ((ch) & 0b10000000)
#define UTF8_IS_CONTINUATION(ch) (((ch) & 0b11000000) == UTF8_CONTINUATION_FLAG)

#define UTF16_IS_HIGH_SURROGATE(ch) (((ch) >> 10) == 0x36)
#define UTF16_IS_LOW_SURROGATE(ch) (((ch) >> 10) == 0x37)

#if SOUP_WINDOWS
#include <windows.h>

#define UTF16_CHAR_TYPE wchar_t
#define UTF16_STRING_TYPE std::wstring
#else
#define UTF16_CHAR_TYPE char16_t
#define UTF16_STRING_TYPE std::u16string
#endif
static_assert(sizeof(UTF16_CHAR_TYPE) == 2);

NAMESPACE_SOUP
{
	struct unicode
	{
		static constexpr uint32_t REPLACEMENT_CHAR = 0xFFFD;

		[[nodiscard]] static char32_t utf8_to_utf32_char(const char*& it, const char* end) noexcept;
		[[nodiscard]] static char32_t utf8_to_utf32_char(std::string::const_iterator& it, const std::string::const_iterator end) noexcept;
#if SOUP_CPP20
		[[nodiscard]] static std::u32string utf8_to_utf32(const char8_t* utf8) noexcept;
#endif
		[[nodiscard]] static std::u32string utf8_to_utf32(const std::string& utf8) noexcept;
#if SOUP_CPP20
		[[nodiscard]] static UTF16_STRING_TYPE utf8_to_utf16(const char8_t* utf8) noexcept;
#endif
		[[nodiscard]] static UTF16_STRING_TYPE utf8_to_utf16(const std::string& utf8) noexcept;
#if SOUP_WINDOWS
		[[nodiscard]] static UTF16_STRING_TYPE acp_to_utf16(const std::string& acp) noexcept;
#endif
		[[nodiscard]] static UTF16_STRING_TYPE utf32_to_utf16(const std::u32string& utf32) noexcept;
		static void utf32_to_utf16_char(UTF16_STRING_TYPE& utf16, char32_t c) noexcept;
		[[nodiscard]] static std::string utf32_to_utf8(char32_t utf32) noexcept;
		[[nodiscard]] static std::string utf32_to_utf8(const std::u32string& utf32) noexcept;

		template <typename Str = std::u16string>
		[[nodiscard]] static char32_t utf16_to_utf32(typename Str::const_iterator& it, const typename Str::const_iterator end) noexcept
		{
			char32_t w1 = static_cast<char32_t>(*it++);
			if (UTF16_IS_HIGH_SURROGATE(w1))
			{
				SOUP_IF_UNLIKELY (it == end)
				{
					return 0;
				}
				char32_t w2 = static_cast<char32_t>(*it++);
				return utf16_to_utf32(w1, w2);
			}
			return w1;
		}

		[[nodiscard]] static char32_t utf16_to_utf32(char32_t hi, char32_t lo) noexcept
		{
			hi &= 0x3FF;
			lo &= 0x3FF;
			return (((hi * 0x400) + lo) + 0x10000);
		}

		template <typename Str>
		[[nodiscard]] static std::u32string utf16_to_utf32(const Str& utf16)
		{
			std::u32string utf32{};
			auto it = utf16.cbegin();
			const auto end = utf16.cend();
			while (it != end)
			{
				auto uni = utf16_to_utf32<Str>(it, end);
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

		template <typename Str = UTF16_STRING_TYPE>
		[[nodiscard]] static std::string utf16_to_utf8(const Str& utf16)
		{
#if SOUP_WINDOWS
			std::string res;
			const int sizeRequired = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)utf16.data(), (int)utf16.size(), NULL, 0, NULL, NULL);
			SOUP_IF_LIKELY (sizeRequired != 0)
			{
				res = std::string(sizeRequired, 0);
				WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)utf16.data(), (int)utf16.size(), res.data(), sizeRequired, NULL, NULL);
			}
			return res;
#else
			return utf32_to_utf8(utf16_to_utf32(utf16));
#endif
		}

		[[nodiscard]] static size_t utf8_char_len(const std::string& str) noexcept;
		[[nodiscard]] static size_t utf16_char_len(const UTF16_STRING_TYPE& str) noexcept;

		template <typename Iterator>
		static void utf8_add(Iterator& it, Iterator end)
		{
			if (UTF8_HAS_CONTINUATION(*it))
			{
				do
				{
					++it;
				} while (it != end && UTF8_IS_CONTINUATION(*it));
			}
			else
			{
				++it;
			}
		}

		template <typename Iterator>
		static void utf8_sub(Iterator& it, Iterator begin)
		{
			--it;
			while (UTF8_IS_CONTINUATION(*it) && it != begin)
			{
				--it;
			}
		}

		static void utf8_sanitise(std::string& str);
		static bool utf8_validate(const char* it, const char* const end);
		static bool utf8_validate(const std::string& str) { return utf8_validate(str.data(), str.data() + str.size()); }
	};
}
