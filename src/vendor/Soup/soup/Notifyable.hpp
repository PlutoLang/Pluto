#pragma once

#include "RecursiveMutex.hpp"

NAMESPACE_SOUP
{
	struct Notifyable
	{
		RecursiveMutex mutex;
#if SOUP_WINDOWS
		CONDITION_VARIABLE cv;
#else
		pthread_cond_t cond;
#endif

		Notifyable() noexcept
		{
#if SOUP_WINDOWS
			InitializeConditionVariable(&cv);
#else
			pthread_cond_init(&cond, nullptr);
#endif
		}

		~Notifyable() noexcept
		{
#if !SOUP_WINDOWS
			pthread_cond_destroy(&cond);
#endif
		}

		// Note: Calling thread may be resumed even if no one called `wakeOne` or `wakeAll`.
		void wait() noexcept
		{
			mutex.lock();
#if SOUP_WINDOWS
			SleepConditionVariableCS(&cv, &mutex.cs, INFINITE);
#else
			pthread_cond_wait(&cond, &mutex.mutex);
#endif
			mutex.unlock();
		}

		void wakeOne() noexcept
		{
#if SOUP_WINDOWS
			WakeConditionVariable(&cv);
#else
			pthread_cond_signal(&cond);
#endif
		}

		void wakeAll() noexcept
		{
#if SOUP_WINDOWS
			WakeAllConditionVariable(&cv);
#else
			pthread_cond_broadcast(&cond);
#endif
		}
	};
}
