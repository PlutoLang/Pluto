#include "Promise.hpp"

#include "os.hpp"

NAMESPACE_SOUP
{
	void PromiseBase::awaitFulfilment()
	{
		while (isPending())
		{
			os::sleep(1);
		}
	}

	void Promise<void>::awaitFulfilment()
	{
		while (isPending())
		{
			os::sleep(1);
		}
	}
}
