#include "Capture.hpp"

#include "Exception.hpp"

namespace soup
{
	void Capture::validate() const
	{
#if SOUP_BITS == 64
		if (reinterpret_cast<uintptr_t>(data) == 0xdddddddddddddddd)
#else
		if (reinterpret_cast<uintptr_t>(data) == 0xdddddddd)
#endif
		{
			throw Exception("Attempt to use Capture after it has been free'd");
		}
	}
}
