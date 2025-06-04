#pragma once

#include "base.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

NAMESPACE_SOUP
{
	struct RecursiveMutex
	{
#if SOUP_WINDOWS
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif

		RecursiveMutex() noexcept
		{
#if SOUP_WINDOWS
			InitializeCriticalSection(&cs);
#else
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&mutex, &attr);
#endif
		}

		~RecursiveMutex() noexcept
		{
#if SOUP_WINDOWS
			DeleteCriticalSection(&cs);
#else
			pthread_mutex_destroy(&mutex);
#endif
		}

		void lock() noexcept
		{
#if SOUP_WINDOWS
			EnterCriticalSection(&cs);
#else
			pthread_mutex_lock(&mutex);
#endif
		}

		[[nodiscard]] bool tryLock() noexcept
		{
#if SOUP_WINDOWS
			return TryEnterCriticalSection(&cs);
#else
			return pthread_mutex_trylock(&mutex) == 0;
#endif
		}

		void unlock() noexcept
		{
#if SOUP_WINDOWS
			LeaveCriticalSection(&cs);
#else
			pthread_mutex_unlock(&mutex);
#endif
		}
	};
}
