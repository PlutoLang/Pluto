#pragma once

#include <ostream>

#define SOUP_STRINGIFYABLE(T) \
friend ::std::ostream& operator<<(::std::ostream& os, const T& v) \
{ \
	return os << v.toString(); \
}
