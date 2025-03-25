#pragma once

#include "RegexFlags.hpp"
#include "RegexGroup.hpp"
#include "RegexMatchResult.hpp"

NAMESPACE_SOUP
{
	struct Regex
	{
		RegexGroup group;

		Regex(const std::string& pattern, const char* flags)
			: Regex(pattern.data(), &pattern.data()[pattern.size()], parseFlags(flags))
		{
		}

		Regex(const std::string& pattern, uint16_t flags = 0)
			: Regex(pattern.data(), &pattern.data()[pattern.size()], flags)
		{
		}

		Regex(const char* it, const char* end, uint16_t flags)
			: group(it, end, flags)
		{
		}

		Regex(const Regex& b)
			: Regex(b.toString(), b.getFlags())
		{
		}

		Regex() = default;

		[[nodiscard]] static Regex fromFullString(const std::string& str);

		[[nodiscard]] bool matches(const std::string& str) const noexcept;
		[[nodiscard]] bool matches(const char* it, const char* end) const noexcept;

		[[nodiscard]] bool matchesFully(const std::string& str) const noexcept;
		[[nodiscard]] bool matchesFully(const char* it, const char* end) const noexcept;

		[[nodiscard]] RegexMatchResult match(const std::string& str) const noexcept;
		[[nodiscard]] RegexMatchResult match(const char* it, const char* end) const noexcept;
		[[nodiscard]] RegexMatchResult match(const char* it, const char* begin, const char* end) const noexcept;
		[[nodiscard]] RegexMatchResult match(RegexMatcher& m, const char* it) const noexcept;

		[[nodiscard]] RegexMatchResult search(const std::string& str) const noexcept;
		[[nodiscard]] RegexMatchResult search(const char* it, const char* end) const noexcept;

		void replaceAll(std::string& str, const std::string& replacement) const;

		[[nodiscard]] std::string substituteAll(const std::string& str, const std::string& substitution) const;

		[[nodiscard]] std::string toString() const SOUP_EXCAL
		{
			return group.toString();
		}

		[[nodiscard]] std::string toFullString() const SOUP_EXCAL
		{
			std::string str(1, '/');
			str.append(toString());
			str.push_back('/');
			str.append(getFlagsString());
			return str;
		}

		[[nodiscard]] uint16_t getFlags() const noexcept
		{
			return group.getFlags();
		}

		[[nodiscard]] std::string getFlagsString() const noexcept
		{
			return unparseFlags(group.getFlags());
		}

		[[nodiscard]] static constexpr uint16_t parseFlags(const char* flags)
		{
			uint16_t res = 0;
			for (; *flags != '\0'; ++flags)
			{
				if (*flags == 'm')
				{
					res |= RE_MULTILINE;
				}
				else if (*flags == 's')
				{
					res |= RE_DOTALL;
				}
				else if (*flags == 'i')
				{
					res |= RE_INSENSITIVE;
				}
				else if (*flags == 'x')
				{
					res |= RE_EXTENDED;
				}
				else if (*flags == 'u')
				{
					res |= RE_UNICODE;
				}
				else if (*flags == 'U')
				{
					res |= RE_UNGREEDY;
				}
				else if (*flags == 'D')
				{
					res |= RE_DOLLAR_ENDONLY;
				}
				else if (*flags == 'n')
				{
					res |= RE_EXPLICIT_CAPTURE;
				}
			}
			return res;
		}

		[[nodiscard]] static std::string unparseFlags(uint16_t flags);

		// Result can be used with 'dot' via CLI to produce an image, or an online viewer such as https://dreampuf.github.io/GraphvizOnline/
		[[nodiscard]] std::string toGraphvizDot() const SOUP_EXCAL;
	};

	namespace literals
	{
		inline Regex operator ""_r(const char* str, size_t len)
		{
			return Regex(std::string(str, len));
		}
	}
}
