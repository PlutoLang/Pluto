#include "DetachedScheduler.hpp"
#if !SOUP_WASM

namespace soup
{
	DetachedScheduler::DetachedScheduler(netConfig&& conf)
		: conf(std::move(conf))
	{
	}

	SharedPtr<Worker> DetachedScheduler::addWorker(SharedPtr<Worker>&& w) noexcept
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
		Callback<void(netConfig&)> cb;

		UpdateConfigTask(void fp(netConfig&, Capture&&), Capture&& cap)
			: cb(fp, std::move(cap))
		{
		}

		void onTick() final
		{
			cb(netConfig::get());
			setWorkDone();
		}
	};

	void DetachedScheduler::updateConfig(void fp(netConfig&, Capture&&), Capture&& cap)
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
		void onTick() final
		{
			Scheduler::get()->closeReusableSockets();
			setWorkDone();
		}
	};

	void DetachedScheduler::closeReusableSockets()
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
