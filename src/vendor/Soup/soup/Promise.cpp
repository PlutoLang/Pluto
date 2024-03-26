#include "Promise.hpp"

#include <thread>

namespace soup
{
	void PromiseBase::awaitFulfilment()
	{
		while (isPending())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void Promise<void>::awaitFulfilment()
	{
		while (isPending())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}
