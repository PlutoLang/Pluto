#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum RegexFlags : uint16_t
	{
		RE_GLOBAL = (1 << 0), // 'g' - high-level flag indicating "don't stop at first match" - unused by Soup itself but can be queried with Regex::hasGlobalFlag
		RE_MULTILINE = (1 << 1), // 'm' - '^' and '$' also match start and end of lines, respectively
		RE_DOTALL = (1 << 2), // 's' - '.' also matches '\n'
		RE_INSENSITIVE = (1 << 3), // 'i' - case insensitive match
		RE_EXTENDED = (1 << 4), // 'x' - Ignore bare space characters in pattern. '#' signifies begin of line comment.
		RE_UNICODE = (1 << 5), // 'u' - Treat pattern and strings-to-match as UTF-8 instead of binary data
		RE_UNGREEDY = (1 << 6), // 'U' - Quantifiers become lazy by default and are instead made greedy by a trailing '?'
		RE_DOLLAR_ENDONLY = (1 << 7), // 'D' - '$' only matches end of pattern, not '\n' - ignored if multi_line flag is set
		RE_EXPLICIT_CAPTURE = (1 << 8), // 'n' - only capture named groups (non-standard flag from .NET/C#)
	};
}
