#pragma once

#include "base.hpp"
#if !SOUP_WASM

#if SOUP_WINDOWS
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <vector>

#include "Capture.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	class Thread
	{
	public:
#if SOUP_WINDOWS
		HANDLE handle = INVALID_HANDLE_VALUE;
#else
		pthread_t handle{};
		bool have_handle = false;
#endif
		bool running = false;
		bool is_self_deleting = false;
		void(*f)(Capture&&);
		Capture f_cap;

		explicit Thread() noexcept = default;
		explicit Thread(void(*f)(Capture&&), Capture&& cap = {});
		explicit Thread(const Thread& b) = delete;
		explicit Thread(Thread&& b) = delete;
		void start(void(*f)(Capture&&), Capture&& cap = {});

		~Thread() noexcept;

#if SOUP_WINDOWS || SOUP_LINUX
		void setTimeCritical() noexcept;
#endif

		[[nodiscard]] bool isRunning() const noexcept { return running; }

		void awaitCompletion() noexcept;
		static void awaitCompletion(const std::vector<UniquePtr<Thread>>& threads) noexcept;
	};
}

#endif
