#pragma once

#include "base.hpp"

#include <stdexcept>

NAMESPACE_SOUP
{
	struct Exception : public std::runtime_error
	{
		using std::runtime_error::runtime_error;

		[[noreturn]] static SOUP_FORCEINLINE void purecall()
		{
			SOUP_THROW(Exception("Call to virtual function that was not implemented by specialisation"));
		}

		[[noreturn]] static SOUP_FORCEINLINE void raiseLogicError()
		{
			SOUP_THROW(Exception("Logic error"));
		}
	};
}
