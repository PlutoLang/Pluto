#pragma once

#include <string>

NAMESPACE_SOUP
{
	struct RegexMatchedGroup
	{
		std::string name;
		const char* begin;
		const char* end;

		[[nodiscard]] size_t length() const
		{
			return std::distance(begin, end);
		}

		[[nodiscard]] std::string toString() const noexcept
		{
			return std::string(begin, end);
		}
	};
}
