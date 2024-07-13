#pragma once

#include "string.hpp"

NAMESPACE_SOUP
{
	class ConstString
	{
	private:
		const char* str;
		size_t len;
		
	public:
		constexpr ConstString(const char* str) noexcept
			: str(str), len(string::len(str))
		{
		}

		[[nodiscard]] bool operator ==(const std::string& b) const noexcept
		{
			return b.length() == len
				&& strcmp(str, b.c_str()) == 0
				;
		}

		[[nodiscard]] bool operator !=(const std::string& b) const noexcept
		{
			return !operator==(b);
		}

		[[nodiscard]] constexpr operator const char* () const noexcept
		{
			return str;
		}

		[[nodiscard]] constexpr const char* c_str() const noexcept
		{
			return str;
		}

		[[nodiscard]] constexpr size_t size() const noexcept
		{
			return len;
		}

		[[nodiscard]] constexpr size_t length() const noexcept
		{
			return len;
		}

		[[nodiscard]] bool isStartOf(const std::string& b) const noexcept
		{
			return b.length() >= length()
				&& memcmp(b.c_str(), c_str(), length()) == 0
				;
		}
	};
}
