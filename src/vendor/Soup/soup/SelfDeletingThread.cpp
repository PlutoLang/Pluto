#include "SelfDeletingThread.hpp"

#include "log.hpp"

namespace soup
{
	SelfDeletingThread::SelfDeletingThread(void(*f)(Capture&&), Capture&& cap) noexcept
		: Thread(), f(f), cap(std::move(cap))
	{
		start(run, this);
	}

	void SelfDeletingThread::run(Capture&& cap)
	{
		auto t = cap.get<SelfDeletingThread*>();
#if SOUP_EXCEPTIONS
		try
#endif
		{
			t->f(std::move(t->cap));
		}
#if SOUP_EXCEPTIONS
		catch (const std::exception& e)
		{
			std::string msg = "Exception in SelfDeletingThread: ";
			msg.append(e.what());
			logWriteLine(std::move(msg));
		}
#endif
		t->is_self_deleting = true;
	}
}
