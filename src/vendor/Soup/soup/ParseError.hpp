#pragma once

#include "Exception.hpp"

NAMESPACE_SOUP
{
	struct ParseError : public Exception
	{
		using Exception::Exception;
	};
}
