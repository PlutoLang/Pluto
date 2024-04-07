#pragma once

#include "Thread.hpp"
#if !SOUP_WASM

NAMESPACE_SOUP
{
	class SelfDeletingThread : public Thread
	{
	public:
		explicit SelfDeletingThread(void(*f)(Capture&&), Capture&& cap = {});

	protected:
		static void run(Capture&& cap);

		void(*f)(Capture&&);
		Capture cap;
	};
}

#endif
