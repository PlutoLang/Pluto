#pragma once

#include "Scheduler.hpp"
#if !SOUP_WASM

#include "netConfig.hpp"
#include "Thread.hpp"

namespace soup
{
	class DetachedScheduler : public Scheduler
	{
	protected:
		netConfig conf;
	public:
		Thread thrd;

		DetachedScheduler(netConfig&& conf = {});
		~DetachedScheduler();

		SharedPtr<Worker> addWorker(SharedPtr<Worker>&& w) noexcept final;

		[[nodiscard]] bool isActive() const noexcept { return thrd.isRunning(); }
		void awaitCompletion() noexcept { return thrd.awaitCompletion(); }

		void updateConfig(void fp(netConfig&, Capture&&), Capture&& cap = {});

		void closeReusableSockets();

	protected:
		virtual void run() { Scheduler::run(); }

	private:
		void threadFunc();
	};
}

#endif
