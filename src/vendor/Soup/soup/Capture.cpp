#include "Capture.hpp"

#include "Exception.hpp"

NAMESPACE_SOUP
{
	void Capture::validate() const
	{
#if SOUP_BITS == 64
		if (reinterpret_cast<uintptr_t>(data) == 0xdddddddddddddddd)
#else
		if (reinterpret_cast<uintptr_t>(data) == 0xdddddddd)
#endif
		{
			SOUP_THROW(Exception("Attempt to use Capture after it has been free'd"));
		}
	}
}
