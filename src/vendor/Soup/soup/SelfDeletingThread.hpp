#pragma once

#include "Thread.hpp"

namespace soup
{
	class SelfDeletingThread : public Thread
	{
	public:
		explicit SelfDeletingThread(void(*f)(Capture&&), Capture&& cap = {}) noexcept;

	protected:
		static void run(Capture&& cap);

		void(*f)(Capture&&);
		Capture cap;
	};
}
