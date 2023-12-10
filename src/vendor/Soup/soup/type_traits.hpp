#pragma once

#include <type_traits>

#define SOUP_RESTRICT(...) std::enable_if_t<(__VA_ARGS__), int> = 0
