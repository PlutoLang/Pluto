#pragma once

#include <string>

#include "spaceship.hpp"

NAMESPACE_SOUP
{
	[[nodiscard]] strong_ordering version_compare(std::string in_a, std::string in_b) SOUP_EXCAL;
}
