#pragma once

#include "base.hpp"

#include <stdexcept>

namespace soup
{
	struct Exception : public std::runtime_error
	{
		using std::runtime_error::runtime_error;

		[[noreturn]] static SOUP_FORCEINLINE void purecall()
		{
			throw Exception("Call to virtual function that was not implemented by specialisation");
		}

		[[noreturn]] static SOUP_FORCEINLINE void raiseLogicError()
		{
			throw Exception("Logic error");
		}
	};
}
