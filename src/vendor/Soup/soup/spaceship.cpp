#include "spaceship.hpp"

#if !SOUP_SPACESHIP_USE_STD
NAMESPACE_SOUP
{
	const strong_ordering strong_ordering::less = -1;
	const strong_ordering strong_ordering::equal = 0;
	const strong_ordering strong_ordering::greater = 1;
}
#endif
