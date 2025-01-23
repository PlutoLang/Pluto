#include "Thread.hpp"
#if !SOUP_WASM

#if !SOUP_WINDOWS
#include <cstring> // memcpy
#endif

#include "Exception.hpp"
#include "format.hpp"
#include "SelfDeletingThread.hpp"

NAMESPACE_SOUP
{
	Thread::Thread(void(*f)(Capture&&), Capture&& cap)
	{
		start(f, std::move(cap));
	}

	Thread::Thread(Thread&& b) noexcept
		:
#if SOUP_WINDOWS
		handle(b.handle),
#else
		have_handle(b.have_handle),
#endif
		running_ref(b.running_ref)
	{
#if !SOUP_WINDOWS
		memcpy(&handle, &b.handle, sizeof(handle));
#endif
		b.forget();
	}

	static void
#if SOUP_WINDOWS
		__stdcall
#endif
		threadCreateCallback(void* handover)
	{
		auto data = reinterpret_cast<Thread::RunningData*>(handover);
		data->f(std::move(data->f_cap));
		data->f_cap.reset();
		delete data;
	}

	void Thread::start(void(*f)(Capture&&), Capture&& cap)
	{
		SOUP_ASSERT(!isRunning());

		// If we still have a handle, relinquish it.
		detach();

		auto data = new RunningData{ f, std::move(cap) };
		this->running_ref = data->transient_token;

#if SOUP_WINDOWS
		handle = CreateThread(nullptr, 0, reinterpret_cast<DWORD(__stdcall*)(LPVOID)>(&threadCreateCallback), data, 0, nullptr);
		SOUP_IF_UNLIKELY (handle == NULL)
		{
			handle = INVALID_HANDLE_VALUE;
			this->running_ref.reset();
			SOUP_THROW(Exception(format("Failed to create thread: {}", GetLastError())));
		}
#else
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		auto ret = pthread_create(&handle, &attr, reinterpret_cast<void*(*)(void*)>(&threadCreateCallback), data);
		SOUP_IF_UNLIKELY (ret != 0)
		{
			this->running_ref.reset();
			SOUP_THROW(Exception(format("Failed to create thread: {}", ret)));
		}
		have_handle = true;
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

	bool Thread::isAttached() const noexcept
	{
#if SOUP_WINDOWS
		return handle != INVALID_HANDLE_VALUE;
#else
		return have_handle;
#endif
	}

	void Thread::awaitCompletion() noexcept
	{
#if SOUP_WINDOWS
		if (handle != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(handle, INFINITE);
			CloseHandle(handle);
			forget();
		}
#else
		if (have_handle)
		{
			pthread_join(handle, nullptr);
			forget();
		}
#endif
	}

	void Thread::awaitCompletion(const std::vector<UniquePtr<Thread>>& threads) noexcept
	{
#if SOUP_WINDOWS
		std::vector<HANDLE> handles{};
		handles.reserve(threads.size());
		for (auto& t : threads)
		{
			if (t->handle != INVALID_HANDLE_VALUE)
			{
				handles.emplace_back(t->handle);
			}
		}
		WaitForMultipleObjects((DWORD)handles.size(), handles.data(), TRUE, INFINITE);
		for (auto& t : threads)
		{
			if (t->handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(t->handle);
				t->forget();
			}
		}
#else
		for (auto& t : threads)
		{
			t->awaitCompletion();
		}
#endif
	}

	void Thread::detach() noexcept
	{
#if SOUP_WINDOWS
		if (handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
			forget();
		}
#else
		if (have_handle)
		{
			pthread_detach(handle);
			forget();
		}
#endif
	}

	void Thread::forget() noexcept
	{
#if SOUP_WINDOWS
		handle = INVALID_HANDLE_VALUE;
#else
		have_handle = false;
#endif
		running_ref.reset();
	}
}

#endif
