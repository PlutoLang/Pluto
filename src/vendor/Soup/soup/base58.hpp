#pragma once

#include "CustomEncoding.hpp"

NAMESPACE_SOUP
{
	using base58 = CustomEncoding<"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz">;
}
