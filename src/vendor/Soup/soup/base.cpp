#include "base.hpp"

static_assert(sizeof(void*) * 8 == SOUP_BITS);

#if !SOUP_EXCEPTIONS
#include <iostream>
#endif

#include "Exception.hpp"
#include "ObfusString.hpp"

NAMESPACE_SOUP
{
#if !SOUP_EXCEPTIONS
	void throwImpl(std::exception&& e)
	{
		std::cerr << e.what();
		abort();
	}
#endif

	void throwAssertionFailed()
	{
		SOUP_THROW(Exception(ObfusString("Assertion failed").str()));
	}

	void throwAssertionFailed(const char* what)
	{
#if false
		std::string msg = "Assertion failed: ";
		msg.append(what);
		SOUP_THROW(Exception(std::move(msg)));
#else
		SOUP_THROW(Exception(what));
#endif
	}
}
