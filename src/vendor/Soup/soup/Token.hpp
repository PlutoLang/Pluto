#pragma once

#include "ConstString.hpp"
#include "Rgb.hpp"

NAMESPACE_SOUP
{
	struct Token
	{
		using parse_t = void(*)(ParserState&);

		ConstString keyword;
		Rgb colour;
		parse_t parse;
		uintptr_t user_data;

		[[nodiscard]] bool operator ==(const char* b) const noexcept
		{
			return keyword.c_str() == b;
		}

		[[nodiscard]] bool operator ==(const std::string& b) const noexcept
		{
			return keyword == b;
		}
	};
}
