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
#define UTF16_LITERAL(...) L"" __VA_ARGS__
#else
#define UTF16_CHAR_TYPE char16_t
#define UTF16_STRING_TYPE std::u16string
#define UTF16_LITERAL(...) u"" __VA_ARGS__
#endif
static_assert(sizeof(UTF16_CHAR_TYPE) == 2);

NAMESPACE_SOUP
{
	struct unicode
	{
		static constexpr uint32_t REPLACEMENT_CHAR = 0xFFFD;
		static constexpr size_t UTF8_MAX_CODEPOINT_LEN = 4; // The maximum number of bytes needed to represent a codepoint in UTF-8.

		[[nodiscard]] static char32_t utf8_to_utf32_char(const char*& it, const char* end) noexcept;
		[[nodiscard]] static char32_t utf8_to_utf32_char(std::string::const_iterator& it, const std::string::const_iterator end) noexcept;
		[[nodiscard]] static std::u32string utf8_to_utf32(const std::string& utf8) SOUP_EXCAL;

		[[nodiscard]] static size_t utf8_to_utf16_len(const char* data, size_t size) noexcept;
		[[nodiscard]] static size_t utf8_to_utf16_len(const std::string& utf8) noexcept { return utf8_to_utf16_len(utf8.data(), utf8.size()); }
		static void utf8_to_utf16(const char* data, size_t size, UTF16_CHAR_TYPE out[/*utf8_to_utf16_len(data, size)*/]) noexcept;
		[[nodiscard]] static UTF16_STRING_TYPE utf8_to_utf16(const char* data, size_t size) SOUP_EXCAL
		{
			UTF16_STRING_TYPE res(utf8_to_utf16_len(data, size), '\0');
			utf8_to_utf16(data, size, res.data());
			return res;
		}
		[[nodiscard]] static UTF16_STRING_TYPE utf8_to_utf16(const std::string& utf8) SOUP_EXCAL { return utf8_to_utf16(utf8.data(), utf8.size()); }

#if SOUP_WINDOWS
		[[nodiscard]] static UTF16_STRING_TYPE acp_to_utf16(const std::string& acp) SOUP_EXCAL;
#endif

		[[nodiscard]] static size_t utf32_to_utf16_len(char32_t utf32) noexcept { return 1 + (utf32 > 0xFFFF); }
		static size_t utf32_to_utf16(char32_t utf32, UTF16_CHAR_TYPE out[]) noexcept;
		[[nodiscard]] static UTF16_STRING_TYPE utf32_to_utf16(const std::u32string& utf32) SOUP_EXCAL;

		[[nodiscard]] static size_t utf32_to_utf8_len(char32_t utf32) noexcept;
		static size_t utf32_to_utf8(char32_t utf32, char out[/*utf32_to_utf8_len(utf32)*/]) noexcept;
		[[nodiscard]] static std::string utf32_to_utf8(char32_t utf32) SOUP_EXCAL;
		[[nodiscard]] static std::string utf32_to_utf8(const std::u32string& utf32) SOUP_EXCAL;

		template <typename Str = std::u16string>
		[[nodiscard]] static char32_t utf16_to_utf32(typename Str::const_iterator& it, const typename Str::const_iterator end) noexcept
		{
			static_assert(sizeof(typename Str::value_type) == 2);

			char32_t w1 = static_cast<char32_t>(*it++);
			if (!UTF16_IS_HIGH_SURROGATE(w1))
			{
				return w1;
			}
			SOUP_IF_LIKELY (it != end)
			{
				char32_t w2 = static_cast<char32_t>(*it);
				SOUP_IF_LIKELY (UTF16_IS_LOW_SURROGATE(w2))
				{
					++it;
					return utf16_to_utf32(w1, w2);
				}
			}
			return REPLACEMENT_CHAR;
		}

		[[nodiscard]] static char32_t utf16_to_utf32(char32_t hi, char32_t lo) noexcept
		{
			hi &= 0x3FF;
			lo &= 0x3FF;
			return (((hi * 0x400) + lo) + 0x10000);
		}

		template <typename Str>
		[[nodiscard]] static std::u32string utf16_to_utf32(const Str& utf16) SOUP_EXCAL
		{
			static_assert(sizeof(typename Str::value_type) == 2);

			std::u32string utf32{};
			auto it = utf16.cbegin();
			const auto end = utf16.cend();
			while (it != end)
			{
				utf32.push_back(utf16_to_utf32<Str>(it, end));
			}
			return utf32;
		}

		[[nodiscard]] static size_t utf16_to_utf8_len(const void* data, size_t size) noexcept; // data must be an array of 2-byte elements, with size being given in number of elements, not bytes.

		template <typename Str = UTF16_STRING_TYPE>
		[[nodiscard]] static size_t utf16_to_utf8_len(const Str& utf16) noexcept
		{
			static_assert(sizeof(typename Str::value_type) == 2);

			return utf16_to_utf8_len(utf16.data(), utf16.size());
		}

		static void utf16_to_utf8(const void* data, size_t size, char out[/*utf16_to_utf8_len(data, size)*/]) noexcept; // data must be an array of 2-byte elements, with size being given in number of elements, not bytes.

		[[nodiscard]] static std::string utf16_to_utf8(const void* data, size_t size) SOUP_EXCAL // data must be an array of 2-byte elements, with size being given in number of elements, not bytes.
		{
			std::string res(utf16_to_utf8_len(data, size), '\0');
			utf16_to_utf8(data, size, res.data());
			return res;
		}

		template <typename Str = UTF16_STRING_TYPE>
		[[nodiscard]] static std::string utf16_to_utf8(const Str& utf16) SOUP_EXCAL
		{
			static_assert(sizeof(typename Str::value_type) == 2);

			return utf16_to_utf8(utf16.data(), utf16.size());
		}

		[[nodiscard]] static size_t utf8_char_len(const std::string& str) noexcept;
		[[nodiscard]] static size_t utf16_char_len(const UTF16_STRING_TYPE& str) noexcept;

		template <typename Iterator>
		static void utf8_add(Iterator& it, Iterator end) noexcept
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
		static void utf8_sub(Iterator& it, Iterator begin) noexcept
		{
			--it;
			while (UTF8_IS_CONTINUATION(*it) && it != begin)
			{
				--it;
			}
		}

		static void utf8_sanitise(std::string& str) SOUP_EXCAL;
		static bool utf8_validate(const char* it, const char* const end) noexcept;
		static bool utf8_validate(const std::string& str) noexcept { return utf8_validate(str.data(), str.data() + str.size()); }
	};

#if SOUP_WINDOWS
	SOUP_FORCEINLINE size_t unicode::utf8_to_utf16_len(const char* data, size_t size) noexcept
	{
		return MultiByteToWideChar(CP_UTF8, 0, data, (int)size, nullptr, 0);
	}

	SOUP_FORCEINLINE size_t unicode::utf16_to_utf8_len(const void* data, size_t size) noexcept
	{
		return WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)data, (int)size, NULL, 0, NULL, NULL);
	}
#endif
}
