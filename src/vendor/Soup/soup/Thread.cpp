#include "Thread.hpp"
#if !SOUP_WASM

#include "Exception.hpp"
#include "format.hpp"
#include "SelfDeletingThread.hpp"

NAMESPACE_SOUP
{
	Thread::Thread(void(*f)(Capture&&), Capture&& cap)
	{
		start(f, std::move(cap));
	}

	static void
#if SOUP_WINDOWS
		__stdcall
#endif
		threadCreateCallback(void* handover)
	{
		auto t = reinterpret_cast<Thread*>(handover);
		t->f(std::move(t->f_cap));
		t->f_cap.reset();
		const bool is_self_deleting = t->is_self_deleting;
		t->running = false;
		if (is_self_deleting)
		{
#if SOUP_WINDOWS
			CloseHandle(t->handle);
			t->handle = INVALID_HANDLE_VALUE;
#else
			pthread_detach(t->handle);
			t->have_handle = false;
#endif
			delete static_cast<SelfDeletingThread*>(t);
		}
	}

	void Thread::start(void(*f)(Capture&&), Capture&& cap)
	{
		SOUP_ASSERT(!isRunning());

		this->f = f;
		this->f_cap = std::move(cap);

#if SOUP_WINDOWS
		// if we still have a handle, relinquish it
		if (handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}

		handle = CreateThread(nullptr, 0, reinterpret_cast<DWORD(__stdcall*)(LPVOID)>(&threadCreateCallback), this, 0, nullptr);
		SOUP_IF_UNLIKELY (handle == NULL)
		{
			handle = INVALID_HANDLE_VALUE;
			SOUP_THROW(Exception(format("Failed to create thread: {}", GetLastError())));
		}
#else
		// if we still have a handle, relinquish it
		awaitCompletion();

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		auto ret = pthread_create(&handle, &attr, reinterpret_cast<void*(*)(void*)>(&threadCreateCallback), this);
		SOUP_IF_UNLIKELY (ret != 0)
		{
			SOUP_THROW(Exception(format("Failed to create thread: {}", ret)));
		}
		have_handle = true;
#endif

		running = true;
	}

	Thread::~Thread() noexcept
	{
#if SOUP_WINDOWS
		if (handle != INVALID_HANDLE_VALUE)
		{
			awaitCompletion();
			CloseHandle(handle);
		}
#else
		awaitCompletion();
#endif
	}

#if SOUP_WINDOWS || SOUP_LINUX
	void Thread::setTimeCritical() noexcept
	{
#if SOUP_WINDOWS
		SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);
#else
		pthread_setschedprio(handle, 15);
#endif
	}
#endif

	void Thread::awaitCompletion() noexcept
	{
#if SOUP_WINDOWS
		if (handle != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(handle, INFINITE);
		}
#else
		if (have_handle)
		{
			pthread_join(handle, nullptr);
			have_handle = false;
		}
#endif
	}

	void Thread::awaitCompletion(const std::vector<UniquePtr<Thread>>& threads) noexcept
	{
#if SOUP_WINDOWS
		std::vector<HANDLE> handles{};
		for (auto& t : threads)
		{
			handles.emplace_back(t->handle);
		}
		WaitForMultipleObjects((DWORD)handles.size(), handles.data(), TRUE, INFINITE);
#else
		for (auto& t : threads)
		{
			t->awaitCompletion();
		}
#endif
	}
}

#endif
