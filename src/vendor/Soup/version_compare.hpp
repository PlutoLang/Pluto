#pragma once

#include <string>

#include "spaceship.hpp"

namespace soup
{
	[[nodiscard]] strong_ordering version_compare(std::string in_a, std::string in_b);
}
