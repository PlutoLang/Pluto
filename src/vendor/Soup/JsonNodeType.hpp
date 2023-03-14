#pragma once

#include <cstdint>

namespace soup
{
	enum JsonNodeType : uint8_t
	{
		JSON_INT = 0,
		JSON_FLOAT,
		JSON_STRING,
		JSON_BOOL,
		JSON_NULL,

		JSON_ARRAY,
		JSON_OBJECT,
	};
}
