#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum BuiltinOp : uint8_t
	{
		OP_PUSH_STR  = 0xFF - 0,
		OP_PUSH_INT  = 0xFF - 1,
		OP_PUSH_UINT = 0xFF - 2,
		OP_PUSH_FUN  = 0xFF - 3,
		OP_PUSH_VAR  = 0xFF - 4,
		OP_POP_ARGS  = 0xFF - 5,
	};
}
