#include "DetachedScheduler.hpp"
#if !SOUP_WASM

namespace soup
{
	DetachedScheduler::DetachedScheduler(netConfig&& conf) noexcept
		: conf(std::move(conf))
	{
	}

	DetachedScheduler::~DetachedScheduler() SOUP_EXCAL
	{
		closeReusableSockets();
	}

	SharedPtr<Worker> DetachedScheduler::addWorker(SharedPtr<Worker>&& w) SOUP_EXCAL
	{
		if (!thrd.isRunning())
		{
			thrd.start([](Capture&& cap)
			{
				cap.get<DetachedScheduler*>()->threadFunc();
			}, this);
		}
		return Scheduler::addWorker(std::move(w));
	}

	struct UpdateConfigTask : public Task
	{
		Callback<void(netConfig&) SOUP_EXCAL> cb;

		UpdateConfigTask(void fp(netConfig&, Capture&&) SOUP_EXCAL, Capture&& cap) noexcept
			: cb(fp, std::move(cap))
		{
		}

		void onTick() SOUP_EXCAL final
		{
			cb(netConfig::get());
			setWorkDone();
		}
	};

	void DetachedScheduler::updateConfig(void fp(netConfig&, Capture&&) SOUP_EXCAL, Capture&& cap) SOUP_EXCAL
	{
		if (isActive())
		{
			add<UpdateConfigTask>(fp, std::move(cap));
		}
		else
		{
			fp(conf, std::move(cap));
		}
	}

	struct CloseReusableSocketsTask : public Task
	{
		void onTick() SOUP_EXCAL final
		{
			Scheduler::get()->closeReusableSockets();
			setWorkDone();
		}
	};

	void DetachedScheduler::closeReusableSockets() SOUP_EXCAL
	{
		if (isActive())
		{
			add<CloseReusableSocketsTask>();
		}
	}

	void DetachedScheduler::threadFunc()
	{
		do
		{
			netConfig::get() = std::move(conf);
			run();
			workers.clear();
			passive_workers = 0;
			conf = std::move(netConfig::get());
		} while (!pending_workers.empty());
	}
}

#endif
