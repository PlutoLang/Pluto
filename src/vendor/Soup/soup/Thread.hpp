#pragma once

#include "base.hpp"
#if !SOUP_WASM

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <vector>

#include "Capture.hpp"
#include "TransientToken.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	// This class itself is not thread-safe. If you need multiple threads to access the same instance, use a mutex.
	class Thread
	{
	public:
		struct RunningData
		{
			void(*f)(Capture&&);
			Capture f_cap;
			TransientToken transient_token;
		};

#if SOUP_WINDOWS
		HANDLE handle = INVALID_HANDLE_VALUE;
#else
		pthread_t handle{};
		bool have_handle = false;
#endif
		TransientTokenRef running_ref{};

		explicit Thread() SOUP_EXCAL = default;
		explicit Thread(void(*f)(Capture&&), Capture&& cap = {});
		explicit Thread(const Thread& b) = delete;
		explicit Thread(Thread&& b) noexcept;
		void start(void(*f)(Capture&&), Capture&& cap = {});

		~Thread() noexcept { awaitCompletion(); }

#if SOUP_WINDOWS || SOUP_LINUX
		void setTimeCritical() noexcept;
#endif

		[[nodiscard]] bool isAttached() const noexcept;
		[[nodiscard]] bool isRunning() const noexcept { return running_ref.isValid(); }

		void awaitCompletion() noexcept;
		static void awaitCompletion(const std::vector<UniquePtr<Thread>>& threads) noexcept;

		void detach() noexcept;

	protected:
		void forget() noexcept;
	};
}

#endif
