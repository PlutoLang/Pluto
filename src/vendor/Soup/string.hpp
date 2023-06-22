#pragma once

#include <algorithm> // transform
#include <cmath> // fmod
#include <cstdint>
#include <cstring> // strlen
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#undef min

#include "base.hpp"

namespace soup
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
				res.insert(0, 1, '0' + digit);
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
		[[nodiscard]] static std::string fdecimal(Float f)
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
				while (str.at(str.size() - 1) == '0')
				{
					str.erase(str.size() - 1);
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
		[[deprecated]] static std::string hex_lower(Int i)
		{
			return hexLower(i);
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

		[[nodiscard]] static std::string bin2hex(const std::string& str)
		{
			return bin2hexImpl(str, charset_hex);
		}

		[[nodiscard]] static std::string bin2hexLower(const std::string& str)
		{
			return bin2hexImpl(str, charset_hex_lower);
		}

		[[nodiscard]] static std::string bin2hexImpl(std::string str, const char* map)
		{
			std::string res{};
			res.reserve(str.size() * 2);
			for (const auto& c : str)
			{
				res.push_back(map[(unsigned char)c >> 4]);
				res.push_back(map[c & 0b1111]);
			}
			return res;
		}
		
		enum ToIntFlags : uint8_t
		{
			TI_FULL = 1 << 0, // The entire string must be processed. If the string is too long or contains invalid characters, nullopt or fallback will be returned.
		};

		template <typename IntT, typename CharT>
		[[nodiscard]] static IntT toIntImpl(const CharT*& it) noexcept
		{
			IntT val = 0;
			IntT max = 0;
			IntT prev_max = 0;
			while (true)
			{
				if constexpr (std::is_unsigned_v<IntT>)
				{
					max *= 10;
					max += 9;
					SOUP_IF_UNLIKELY (!(max > prev_max))
					{
						break;
					}
					prev_max = max;
				}

				const CharT c = *it++;
				SOUP_IF_UNLIKELY (!isNumberChar(c))
				{
					--it;
					break;
				}
				val *= 10;
				val += (c - '0');

				if constexpr (std::is_signed_v<IntT>)
				{
					max *= 10;
					max += 9;
					SOUP_IF_UNLIKELY (max < prev_max)
					{
						break;
					}
					prev_max = max;
				}
			}
			return val;
		}

		template <typename IntT, typename CharT>
		[[nodiscard]] static std::optional<IntT> toIntOpt(const CharT*& it) noexcept
		{
			bool had_number_char = false;
			IntT val = 0;
			IntT max = 0;
			IntT prev_max = 0;
			while (true)
			{
				if constexpr (std::is_unsigned_v<IntT>)
				{
					max *= 10;
					max += 9;
					SOUP_IF_UNLIKELY (!(max > prev_max))
					{
						break;
					}
					prev_max = max;
				}

				const CharT c = *it++;
				SOUP_IF_UNLIKELY (!isNumberChar(c))
				{
					--it;
					break;
				}
				had_number_char = true;
				val *= 10;
				val += (c - '0');

				if constexpr (std::is_signed_v<IntT>)
				{
					max *= 10;
					max += 9;
					SOUP_IF_UNLIKELY (max < prev_max)
					{
						break;
					}
					prev_max = max;
				}
			}
			if (!had_number_char)
			{
				return std::nullopt;
			}
			return val;
		}

		template <typename IntT, uint8_t Flags = 0, typename CharT>
		[[nodiscard]] static std::optional<IntT> toInt(const CharT* it) noexcept
		{
			bool neg = false;
			if (*it == '\0')
			{
				return std::nullopt;
			}
			switch (*it)
			{
			case '-':
				neg = true;
				[[fallthrough]];
			case '+':
				if (*++it == '\0')
				{
					return std::nullopt;
				}
			}
			if (!isNumberChar(*it))
			{
				return std::nullopt;
			}
			IntT val = toIntImpl<IntT, CharT>(it);
			if constexpr (Flags & TI_FULL)
			{
				if (*it != '\0')
				{
					return std::nullopt;
				}
			}
			if (neg)
			{
				val *= -1;
			}
			return std::optional<IntT>(std::move(val));
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static std::optional<IntT> toInt(const std::string& str) noexcept
		{
			return toInt<IntT, Flags>(str.c_str());
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static std::optional<IntT> toInt(const std::wstring& str) noexcept
		{
			return toInt<IntT, Flags>(str.c_str());
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT toInt(const char* str, IntT fallback) noexcept
		{
			auto res = toInt<IntT, Flags>(str);
			if (res.has_value())
			{
				return res.value();
			}
			return fallback;
		}
		
		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT toInt(const std::string& str, IntT fallback) noexcept
		{
			return toInt<IntT, Flags>(str.c_str(), fallback);
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT toInt(const wchar_t* str, IntT fallback) noexcept
		{
			auto res = toInt<IntT, Flags>(str);
			if (res.has_value())
			{
				return res.value();
			}
			return fallback;
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT toInt(const std::wstring& str, IntT fallback) noexcept
		{
			return toInt<IntT, Flags>(str.c_str(), fallback);
		}

		template <typename IntT, typename CharT>
		[[nodiscard]] static IntT hexToIntImpl(const CharT*& it)
		{
			IntT val = 0;
			IntT max = 0;
			IntT prev_max = 0;
			while (true)
			{
				if constexpr (std::is_unsigned_v<IntT>)
				{
					max *= 0x10;
					max += 0xf;
					SOUP_IF_UNLIKELY (!(max > prev_max))
					{
						break;
					}
					prev_max = max;
				}

				const CharT c = *it++;
				if (isNumberChar(c))
				{
					val *= 0x10;
					val += (c - '0');
				}
				else if (c >= 'a' && c <= 'f')
				{
					val *= 0x10;
					val += 0xA + (c - 'a');
				}
				else if (c >= 'A' && c <= 'F')
				{
					val *= 0x10;
					val += 0xA + (c - 'A');
				}
				else
				{
					--it;
					break;
				}

				if constexpr (std::is_signed_v<IntT>)
				{
					max *= 0x10;
					max += 0xf;
					SOUP_IF_UNLIKELY (max < prev_max)
					{
						break;
					}
					prev_max = max;
				}
			}
			return val;
		}

		template <typename IntT, uint8_t Flags = 0, typename CharT>
		[[nodiscard]] static std::optional<IntT> hexToInt(const CharT* it) noexcept
		{
			if (*it == '\0')
			{
				return std::nullopt;
			}
			if (!isHexDigitChar(*it))
			{
				return std::nullopt;
			}
			IntT val = hexToIntImpl<IntT, CharT>(it);
			if constexpr (Flags & TI_FULL)
			{
				if (*it != '\0')
				{
					return std::nullopt;
				}
			}
			return std::optional<IntT>(std::move(val));
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static std::optional<IntT> hexToInt(const std::string& str) noexcept
		{
			return hexToInt<IntT, Flags>(str.c_str());
		}

		template <typename IntT, uint8_t Flags>
		[[nodiscard]] static std::optional<IntT> hexToInt(const std::wstring& str) noexcept
		{
			return hexToInt<IntT, Flags>(str.c_str());
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT hexToInt(const char* str, IntT fallback) noexcept
		{
			auto res = hexToInt<IntT, Flags>(str);
			if (res.has_value())
			{
				return res.value();
			}
			return fallback;
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT hexToInt(const std::string& str, IntT fallback) noexcept
		{
			return hexToInt<IntT, Flags>(str.c_str(), fallback);
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT hexToInt(const wchar_t* str, IntT fallback) noexcept
		{
			auto res = hexToInt<IntT, Flags>(str);
			if (res.has_value())
			{
				return res.value();
			}
			return fallback;
		}

		template <typename IntT, uint8_t Flags = 0>
		[[nodiscard]] static IntT hexToInt(const std::wstring& str, IntT fallback) noexcept
		{
			return hexToInt<IntT, Flags>(str.c_str(), fallback);
		}

		// string mutation

		template <class S>
		[[deprecated]] static void replace_all(S& str, const S& from, const S& to) noexcept
		{
			replaceAll(str, from, to);
		}

		[[deprecated]] static void replace_all(std::string& str, const std::string& from, const std::string& to) noexcept
		{
			return replaceAll<std::string>(str, from, to);
		}

		[[deprecated]] static void replace_all(std::wstring& str, const std::wstring& from, const std::wstring& to) noexcept
		{
			return replaceAll<std::wstring>(str, from, to);
		}

		template <class S>
		static void replaceAll(S& str, const S& from, const S& to) noexcept
		{
			size_t start_pos = 0;
			while ((start_pos = str.find(from, start_pos)) != S::npos)
			{
				str.replace(start_pos, from.length(), to);
				start_pos += to.length();
			}
		}

		static void replaceAll(std::string& str, const std::string& from, const std::string& to) noexcept
		{
			return replaceAll<std::string>(str, from, to);
		}

		static void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) noexcept
		{
			return replaceAll<std::wstring>(str, from, to);
		}

		template <class S>
		[[nodiscard]] static S replaceAll(const S& str, const S& from, const S& to) noexcept
		{
			S cpy(str);
			replaceAll(cpy, from, to);
			return cpy;
		}

		[[nodiscard]] static std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) noexcept
		{
			return replaceAll<std::string>(str, from, to);
		}

		[[nodiscard]] static std::wstring replaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to) noexcept
		{
			return replaceAll<std::wstring>(str, from, to);
		}

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
		[[nodiscard]] static std::vector<S> explode(const S& str, D delim)
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
				if (remain_len != 0)
				{
					res.emplace_back(str.substr(prev, remain_len));
				}
				else
				{
					if constexpr (std::is_same_v<D, char> || std::is_same_v<D, wchar_t>)
					{
						if (str.at(str.length() - 1) == delim)
						{
							res.emplace_back(S{});
						}
					}
				}
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

		static void listAppend(std::string& str, std::string&& add);

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
				auto i = str.cbegin();
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
				auto i = (str.cend() - 1);
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
		[[nodiscard]] static std::string fixType(std::u8string str)
		{
			std::string fixed = std::move(*reinterpret_cast<std::string*>(&str));
			return fixed;
		}

		[[nodiscard]] static std::u8string toUtf8Type(std::string str)
		{
			std::u8string u8 = std::move(*reinterpret_cast<std::u8string*>(&str));
			return u8;
		}
#endif

		// char mutation

		template <typename Str>
		static void lower(Str& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](typename Str::value_type c) -> typename Str::value_type
			{
				if (c >= 'A' && c <= 'Z')
				{
					return c - 'A' + 'a';
				}
				return c;
			});
		}

		template <typename Str>
		[[nodiscard]] static Str lower(Str&& str)
		{
			lower(str);
			return str;
		}

		template <typename Str>
		static void upper(Str& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](typename Str::value_type c) -> typename Str::value_type
			{
				if (c >= 'a' && c <= 'z')
				{
					return c - 'a' + 'A';
				}
				return c;
			});
		}

		template <typename Str>
		[[nodiscard]] static Str upper(Str&& str)
		{
			upper(str);
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

		[[nodiscard]] static std::string fromFile(const std::string& file);
		[[nodiscard]] static std::string fromFilePath(const std::filesystem::path& file);
	};
}
