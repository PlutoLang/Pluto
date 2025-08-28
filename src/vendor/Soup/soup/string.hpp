#pragma once

#include <algorithm> // transform
#include <cmath> // fmod
#include <cstdint>
#include <cstring> // strlen
#include <filesystem>
#include <string>
#include <vector>

#include "base.hpp"
#include "Optional.hpp"

#undef min

NAMESPACE_SOUP
{
	class string
	{
		// from int

	private:
		template <typename Str, typename Int, uint8_t Base>
		[[nodiscard]] static Str fromIntImplAscii(Int _i)
		{
			if (_i == 0)
			{
				return Str(1, '0');
			}
			using UInt = std::make_unsigned_t<Int>;
			UInt i;
			bool neg = false;
			if constexpr (std::is_signed_v<Int>)
			{
				neg = (_i < 0);
				if (neg)
				{
					i = (_i * -1);
				}
				else
				{
					i = _i;
				}
			}
			else
			{
				i = _i;
			}
			Str res{};
			for (; i != 0; i /= Base)
			{
				const auto digit = (i % Base);
				res.insert(0, 1, static_cast<typename Str::value_type>('0' + digit));
			}
			if (neg)
			{
				res.insert(0, 1, '-');
			}
			return res;
		}

	public:
		template <typename Str = std::string, typename Int>
		[[nodiscard]] static Str decimal(Int i) // prefer std::to_string if possible as it's more optimsed
		{
			return fromIntImplAscii<Str, Int, 10>(i);
		}

		template <typename Float>
		[[nodiscard]] static std::string fdecimal(Float f) SOUP_EXCAL
		{
			if (std::fmod(f, 1) == 0)
			{
				std::string str = std::to_string((long long)f);
				str.append(".0");
				return str;
			}
			else
			{
				std::string str = std::to_string(f);
				while (str.back() == '0')
				{
					str.pop_back();
				}
				if (str.back() == '.')
				{
					str.push_back('0');
				}
				return str;
			}
		}

		template <typename Str = std::string, typename Int>
		[[nodiscard]] static Str binary(Int i)
		{
			return fromIntImplAscii<Str, Int, 2>(i);
		}

		template <typename Int>
		[[nodiscard]] static std::string hex(Int i)
		{
			return fromIntWithMap<std::string, Int, 16>(i, charset_hex);
		}

		template <typename Int>
		[[nodiscard]] static std::string hexLower(Int i)
		{
			return fromIntWithMap<std::string, Int, 16>(i, charset_hex_lower);
		}

		static constexpr const char* charset_hex = "0123456789ABCDEF";
		static constexpr const char* charset_hex_lower = "0123456789abcdef";

		template <typename Str, typename Int, uint8_t Base>
		[[nodiscard]] static Str fromIntWithMap(Int i, const typename Str::value_type* map)
		{
			if (i == 0)
			{
				return Str(1, map[0]);
			}
			const bool neg = (i < 0);
			if (neg)
			{
				i = i * -1;
			}
			Str res{};
			for (; i != 0; i /= Base)
			{
				const auto digit = (i % Base);
				res.insert(0, 1, map[digit]);
			}
			if (neg)
			{
				res.insert(0, 1, '-');
			}
			return res;
		}

		// char attributes

		template <typename T>
		[[nodiscard]] static constexpr bool isUppercaseLetter(const T c) noexcept
		{
			return c >= 'A' && c <= 'Z';
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isLowercaseLetter(const T c) noexcept
		{
			return c >= 'a' && c <= 'z';
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isLetter(const T c) noexcept
		{
			return isUppercaseLetter(c) || isLowercaseLetter(c);
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isSpace(const T c) noexcept
		{
			return c == ' ' || c == '\t' || c == '\n' || c == '\r';
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isNumberChar(const T c) noexcept
		{
			return c >= '0' && c <= '9';
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isAlphaNum(const T c) noexcept
		{
			return isLetter(c) || isNumberChar(c);
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isHexDigitChar(const T c) noexcept
		{
			return isNumberChar(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
		}

		template <typename T>
		[[nodiscard]] static constexpr bool isWordChar(const T c) noexcept
		{
			return isLetter(c) || isNumberChar(c) || c == '_';
		}

		// string attributes

		template <typename T>
		[[nodiscard]] static bool isNumeric(const T& str)
		{
			for (const auto& c : str)
			{
				if (!isNumberChar(c))
				{
					return false;
				}
			}
			return true;
		}

		template <typename T>
		[[nodiscard]] static bool containsWord(const T& haystack, const T& needle)
		{
			if (!needle.empty())
			{
				for (size_t i, off = 0; i = haystack.find(needle, off), i != T::npos; off = i + needle.size())
				{
					if ((i == 0 || !isLetter(haystack.at(i - 1)))
						&& (i + needle.size() == haystack.size() || !isLetter(haystack.at(i + needle.size())))
						)
					{
						return true;
					}
				}
			}
			return false;
		}

		template <typename T = std::string>
		[[nodiscard]] static bool equalsIgnoreCase(const T& a, const T& b)
		{
			if (a.size() != b.size())
			{
				return false;
			}
			for (size_t i = 0; i != a.size(); ++i)
			{
				if (std::tolower(a.at(i)) != std::tolower(b.at(i)))
				{
					return false;
				}
			}
			return true;
		}

		template <typename T = std::string>
		[[nodiscard]] static size_t levenshtein(const T& a, const T& b)
		{
			// Adapted from https://github.com/guilhermeagostinelli/levenshtein/blob/master/levenshtein.cpp & https://gist.github.com/TheRayTracer/2644387

			size_t n = a.size() + 1;
			size_t m = b.size() + 1;

			auto d = new size_t[n * m];

			//memset(d, 0, n * m * sizeof(size_t));
			for (size_t i = 0; i != n; ++i)
			{
				d[0 * n + i] = i;
			}
			for (size_t j = 0; j != m; ++j)
			{
				d[j * n + 0] = j;
			}

			for (size_t i = 1, im = 0; i < m; ++i, ++im)
			{
				for (size_t j = 1, jn = 0; j < n; ++j, ++jn)
				{
					size_t cost = (a[jn] == b[im]) ? 0 : 1;

					if (a[jn] == b[im])
					{
						d[(i * n) + j] = d[((i - 1) * n) + (j - 1)];
					}
					else
					{
						d[(i * n) + j] = std::min(
							std::min(d[(i - 1) * n + j], d[i * n + (j - 1)]) + 1,
							d[(i - 1) * n + (j - 1)] + cost
						);
					}
				}
			}

			const auto r = d[n * m - 1];

			delete[] d;

			return r;
		}

		// conversions

		[[nodiscard]] static std::string bin2hex(const std::string& str, bool spaces = false) SOUP_EXCAL
		{
			return bin2hexImpl(str.data(), str.size(), spaces, charset_hex);
		}

		[[nodiscard]] static std::string bin2hex(const char* data, size_t size, bool spaces = false) SOUP_EXCAL
		{
			return bin2hexImpl(data, size, spaces, charset_hex);
		}

		[[nodiscard]] static std::string bin2hexLower(const std::string& str, bool spaces = false) SOUP_EXCAL
		{
			return bin2hexImpl(str.data(), str.size(), spaces, charset_hex_lower);
		}

		[[nodiscard]] static std::string bin2hexLower(const char* data, size_t size, bool spaces = false) SOUP_EXCAL
		{
			return bin2hexImpl(data, size, spaces, charset_hex_lower);
		}

		[[nodiscard]] static std::string bin2hexImpl(const char* data, size_t size, bool spaces, const char* map) SOUP_EXCAL
		{
			std::string res{};
			res.reserve(size * (2 + spaces));
			for (; size; ++data, --size)
			{
				res.push_back(map[(unsigned char)(*data) >> 4]);
				res.push_back(map[(*data) & 0b1111]);
				if (spaces)
				{
					res.push_back(' ');
				}
			}
			if (spaces && !res.empty())
			{
				res.pop_back();
			}
			return res;
		}

		static void bin2hexAt(char* out, const char* data, size_t size, const char* map) noexcept
		{
			for (; size; ++data, --size)
			{
				*out++ = map[(unsigned char)(*data) >> 4];
				*out++ = map[(*data) & 0b1111];
			}
		}

		[[nodiscard]] static constexpr size_t bin2hexWithSpacesSize(size_t size) noexcept
		{
			return size != 0 ? (size * 3) - 1 : 0;
		}

		static void bin2hexWithSpaces(char* out, const char* data, size_t size, const char* map) noexcept
		{
			for (; size; ++data)
			{
				*out++ = map[(unsigned char)(*data) >> 4];
				*out++ = map[(*data) & 0b1111];
				if (--size)
				{
					*out++ = ' ';
				}
			}
		}

		[[nodiscard]] static std::string hex2bin(const std::string& hex) SOUP_EXCAL { return hex2bin(hex.data(), hex.size()); }
		[[nodiscard]] static std::string hex2bin(const char* data, size_t size) SOUP_EXCAL;

		enum ToIntFlags : uint8_t
		{
			TI_FULL = 1 << 0, // The entire string must be processed. If the string is too long or contains invalid characters, nullopt or fallback will be returned.
		};

		template <typename IntT, uint8_t Base = 10, typename CharT>
		[[nodiscard]] static Optional<IntT> toIntEx(const CharT* it, uint8_t flags = 0, const CharT** end = nullptr) noexcept
		{
			bool neg = false;
			if (*it == '\0')
			{
			_fail:
				if (end)
				{
					*end = it;
				}
				return std::nullopt;
			}
			switch (*it)
			{
			case '-':
				if constexpr (std::is_unsigned_v<IntT>)
				{
					goto _fail;
				}
				neg = true;
				[[fallthrough]];
			case '+':
				if (*++it == '\0')
				{
					goto _fail;
				}
			}
			IntT val = 0;
			{
				bool had_number_char = false;
				IntT max = 0;
				IntT prev_max = 0;
				while (true)
				{
					if constexpr (std::is_unsigned_v<IntT>)
					{
						max *= Base;
						max += (Base - 1);
						SOUP_IF_UNLIKELY (!(max > prev_max))
						{
							break;
						}
						prev_max = max;
					}

					const CharT c = *it;
					if (isNumberChar(c))
					{
						val *= Base;
						val += (c - '0');
					}
					else if (Base > 10 && c >= 'a' && c <= ('a' + Base - 11))
					{
						val *= Base;
						val += 0xA + (c - 'a');
					}
					else if (Base > 10 && c >= 'A' && c <= ('A' + Base - 11))
					{
						val *= Base;
						val += 0xA + (c - 'A');
					}
					else
					{
						break;
					}
					++it;
					had_number_char = true;

					if constexpr (std::is_signed_v<IntT>)
					{
						max *= Base;
						max += (Base - 1);
						SOUP_IF_UNLIKELY (max < prev_max)
						{
							break;
						}
						prev_max = max;
					}
				}
				if (!had_number_char)
				{
					goto _fail;
				}
			}
			if (flags & TI_FULL)
			{
				if (*it != '\0')
				{
					goto _fail;
				}
			}
			if constexpr (std::is_signed_v<IntT>)
			{
				if (neg)
				{
					val *= -1;
				}
			}
			if (end)
			{
				*end = it;
			}
			return Optional<IntT>(val);
		}

		template <typename IntT>
		[[nodiscard]] static Optional<IntT> toIntOpt(const std::string& str, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str.c_str(), flags);
		}

		template <typename IntT>
		[[nodiscard]] static Optional<IntT> toIntOpt(const std::wstring& str, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str.c_str(), flags);
		}

		template <typename IntT>
		[[nodiscard]] static IntT toInt(const char* str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str, flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT toInt(const std::string& str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str.c_str(), flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT toInt(const wchar_t* str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str, flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT toInt(const std::wstring& str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT>(str.c_str(), flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static Optional<IntT> hexToIntOpt(const char* str, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str, flags);
		}

		template <typename IntT>
		[[nodiscard]] static Optional<IntT> hexToIntOpt(const std::string& str, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str.c_str(), flags);
		}

		template <typename IntT>
		[[nodiscard]] static Optional<IntT> hexToIntOpt(const std::wstring& str, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str.c_str(), flags);
		}

		template <typename IntT>
		[[nodiscard]] static IntT hexToInt(const char* str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str, flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT hexToInt(const std::string& str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str.c_str(), flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT hexToInt(const wchar_t* str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str, flags).value_or(fallback);
		}

		template <typename IntT>
		[[nodiscard]] static IntT hexToInt(const std::wstring& str, IntT fallback, uint8_t flags = 0) noexcept
		{
			return toIntEx<IntT, 0x10>(str.c_str(), flags).value_or(fallback);
		}

		// string mutation

		template <class S>
		static void replaceAll(S& str, const S& from, const S& to) SOUP_EXCAL
		{
			size_t pos = 0;
			while ((pos = str.find(from, pos)) != S::npos)
			{
				str.replace(pos, from.length(), to);
				pos += to.length();
			}
		}

		static void replaceAll(std::string& str, char from, char to) SOUP_EXCAL;

		static void replaceAll(std::string& str, const std::string& from, const std::string& to) SOUP_EXCAL
		{
			return replaceAll<std::string>(str, from, to);
		}

		static void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) SOUP_EXCAL
		{
			return replaceAll<std::wstring>(str, from, to);
		}

		template <class S>
		[[nodiscard]] static S replaceAll(const S& str, const S& from, const S& to) SOUP_EXCAL
		{
			S cpy(str);
			replaceAll(cpy, from, to);
			return cpy;
		}

		[[nodiscard]] static std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) SOUP_EXCAL
		{
			return replaceAll<std::string>(str, from, to);
		}

		[[nodiscard]] static std::wstring replaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to) SOUP_EXCAL
		{
			return replaceAll<std::wstring>(str, from, to);
		}

		[[nodiscard]] static std::string escape(const std::string& str);

		template <typename S>
		static constexpr size_t len(S str) noexcept
		{
			if constexpr (std::is_same_v<S, char> || std::is_same_v<S, wchar_t>)
			{
				return 1;
			}
			else if constexpr (std::is_pointer_v<S>)
			{
				size_t len = 0;
				while (str[len] != 0)
				{
					++len;
				}
				return len;
			}
			else
			{
				return str.size();
			}
		}

		template <typename S, typename D>
		[[nodiscard]] static std::vector<S> explode(const S& str, D delim) SOUP_EXCAL
		{
			std::vector<S> res{};
			if (!str.empty())
			{
				res.reserve(5);
				size_t prev = 0;
				size_t del_pos;
				while ((del_pos = str.find(delim, prev)) != std::string::npos)
				{
					res.emplace_back(str.substr(prev, del_pos - prev));
					prev = del_pos + len(delim);
				}
				auto remain_len = (str.length() - prev);
				res.emplace_back(str.substr(prev, remain_len));
			}
			return res;
		}

		[[nodiscard]] static std::string join(const std::vector<std::string>& arr, const char glue);
		[[nodiscard]] static std::string join(const std::vector<std::string>& arr, const std::string& glue);

		template <typename S, typename C>
		static S lpad(S&& str, size_t desired_len, C pad_char)
		{
			lpad(str, desired_len, std::move(pad_char));
			return str;
		}

		template <typename S, typename C>
		static void lpad(S& str, size_t desired_len, C pad_char)
		{
			if (auto diff = desired_len - str.length(); diff > 0)
			{
				str.insert(0, diff, pad_char);
			}
		}

		template <typename S, typename C>
		static S rpad(S&& str, size_t desired_len, C pad_char)
		{
			rpad(str, desired_len, std::move(pad_char));
			return str;
		}

		template <typename S, typename C>
		static void rpad(S& str, size_t desired_len, C pad_char)
		{
			if (auto diff = desired_len - str.length(); diff > 0)
			{
				str.append(diff, pad_char);
			}
		}

		// example:
		// in str = "a b c"
		// target = " "
		// out str = "abc"
		template <typename T>
		static void erase(T& str, const T& target)
		{
			for (size_t i = 0; i = str.find(target, i), i != T::npos; )
			{
				str.erase(i, target.size());
			}
		}

		// example:
		// in str = "a b c"
		// target = " "
		// out str = "a"
		template <typename T>
		static void limit(T& str, const T& target)
		{
			if (size_t i = str.find(target); i != T::npos)
			{
				str.erase(i);
			}
		}

		// example:
		// in str = "a b c"
		// target = " "
		// out str = "a b"
		template <typename T>
		static void limitLast(T& str, const T& target)
		{
			if (size_t i = str.find_last_of(target); i != T::npos)
			{
				str.erase(0, i);
			}
		}

		static void listAppend(std::string& str, std::string add);

		template <typename T>
		static void trim(T& str)
		{
			ltrim(str);
			rtrim(str);
		}

		template <typename T>
		static void ltrim(T& str)
		{
			while (!str.empty())
			{
				auto i = str.begin();
				const char c = *i;
				if (!isSpace(c))
				{
					return;
				}
				str.erase(i);
			}
		}

		template <typename T>
		static void rtrim(T& str)
		{
			while (!str.empty())
			{
				auto i = (str.end() - 1);
				const char c = *i;
				if (!isSpace(c))
				{
					return;
				}
				str.erase(i);
			}
		}

		[[nodiscard]] static std::string _xor(const std::string& l, const std::string& r); // Did you know that "xor" was a C++ keyword?
		[[nodiscard]] static std::string xorSameLength(const std::string& l, const std::string& r);

#if SOUP_CPP20
		[[nodiscard]] static std::string fixType(std::u8string str) noexcept
		{
			std::string fixed = std::move(*reinterpret_cast<std::string*>(&str));
			return fixed;
		}

		[[nodiscard]] static std::u8string toUtf8Type(std::string str) noexcept
		{
			std::u8string u8 = std::move(*reinterpret_cast<std::u8string*>(&str));
			return u8;
		}
#else
		[[nodiscard]] static std::string fixType(std::string str) noexcept
		{
			return str;
		}
#endif

		template <typename T>
		static void truncateWithEllipsis(T& str, size_t max_len)
		{
			SOUP_DEBUG_ASSERT(max_len >= 3);
			if (str.size() > max_len)
			{
				str.resize(max_len);
				auto* data = str.data();
				data[max_len - 3] = '.';
				data[max_len - 2] = '.';
				data[max_len - 1] = '.';
			}
		}

		template <typename T>
		[[nodiscard]] static T truncateWithEllipsis(T&& str, size_t max_len)
		{
			truncateWithEllipsis(str, max_len);
			return str;
		}

		// char mutation

		template <typename Char>
		[[nodiscard]] static Char lower_char(Char c) noexcept
		{
			if (c >= 'A' && c <= 'Z')
			{
				return c - 'A' + 'a';
			}
			return c;
		}

		template <typename Str>
		static void lower(Str& str) noexcept
		{
			std::transform(str.begin(), str.end(), str.begin(), &lower_char<typename Str::value_type>);
		}

		template <typename Str>
		[[nodiscard]] static Str lower(Str&& str) noexcept
		{
			lower(str);
			return str;
		}

		template <typename Char>
		[[nodiscard]] static Char upper_char(Char c) noexcept
		{
			if (c >= 'a' && c <= 'z')
			{
				return c - 'a' + 'A';
			}
			return c;
		}

		template <typename Str>
		static void upper(Str& str) noexcept
		{
			std::transform(str.begin(), str.end(), str.begin(), &upper_char<typename Str::value_type>);
		}

		template <typename Str>
		[[nodiscard]] static Str upper(Str&& str) noexcept
		{
			upper(str);
			return str;
		}

		// "hello world" -> "Hello World"
		template <typename Str>
		static void title(Str& str)
		{
			bool first = true;
			for (auto& c : str)
			{
				if (first)
				{
					first = false;
					c = upper_char(c);
				}
				else
				{
					c = lower_char(c);
				}
				if (isSpace(c))
				{
					first = true;
				}
			}
		}

		template <typename Str>
		[[nodiscard]] static Str title(Str&& str)
		{
			title(str);
			return str;
		}

		[[nodiscard]] static constexpr char rot13(char c) noexcept
		{
			if (isUppercaseLetter(c))
			{
				char val = (c - 'A');
				val += 13;
				if (val >= 26)
				{
					val -= 26;
				}
				return (val + 'A');
			}
			if (isLowercaseLetter(c))
			{
				char val = (c - 'a');
				val += 13;
				if (val >= 26)
				{
					val -= 26;
				}
				return (val + 'a');
			}
			return c;
		}

		// file

		[[nodiscard]] static std::string fromFile(const char* file);
		[[nodiscard]] static std::string fromFile(const std::string& file);
		[[nodiscard]] static std::string fromFile(const std::filesystem::path& file);
		[[deprecated("Replace 'fromFilePath' with 'fromFile'")]] inline static std::string fromFilePath(const std::filesystem::path& file) { return fromFile(file); }
		static void toFile(const char* file, const std::string& contents);
		static void toFile(const std::string& file, const std::string& contents);
		static void toFile(const std::filesystem::path& file, const std::string& contents) { return toFile(file, contents.data(), contents.size()); }
		static void toFile(const std::filesystem::path& file, const char* data, size_t size);
	};
}
