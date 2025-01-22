#include "SelfDeletingThread.hpp"
#if !SOUP_WASM

#include "log.hpp"

NAMESPACE_SOUP
{
	SelfDeletingThread::SelfDeletingThread(void(*f)(Capture&&), Capture&& cap)
		: Thread(), f(f), cap(std::move(cap))
	{
		start(run, this);
	}

	void SelfDeletingThread::run(Capture&& cap)
	{
		auto t = cap.get<SelfDeletingThread*>();
		SOUP_TRY
		{
			t->f(std::move(t->cap));
		}
		SOUP_CATCH (std::exception, e)
		{
			std::string msg = "Exception in SelfDeletingThread: ";
			msg.append(e.what());
			logWriteLine(std::move(msg));
		}
		t->detach();
		delete t;
	}
}

#endif
