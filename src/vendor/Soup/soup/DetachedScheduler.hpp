#pragma once

#include "Scheduler.hpp"
#if !SOUP_WASM

#include "netConfig.hpp"
#include "Thread.hpp"

NAMESPACE_SOUP
{
	class DetachedScheduler : public Scheduler
	{
	protected:
		netConfig conf;
	public:
		Thread thrd;

		DetachedScheduler(netConfig&& conf = {}) noexcept;
		~DetachedScheduler() SOUP_EXCAL;

		void addWorker(SharedPtr<Worker>&& w) SOUP_EXCAL final;

		[[nodiscard]] bool isActive() const noexcept { return thrd.isRunning(); }
		void awaitCompletion() noexcept { return thrd.awaitCompletion(); }

		void updateConfig(void fp(netConfig&, Capture&&) SOUP_EXCAL, Capture&& cap = {}) SOUP_EXCAL;

		void closeReusableSockets() SOUP_EXCAL;

	protected:
		virtual void run() { Scheduler::run(); }

	private:
		void threadFunc();
	};
}

#endif
