#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	struct HttpEmscriptenBuffer
	{
		std::unordered_map<std::string, std::string> header_fields;
		std::vector<const char*> headers;
	};
}
