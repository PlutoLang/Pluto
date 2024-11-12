#include "DetachedScheduler.hpp"
#if !SOUP_WASM

#include "log.hpp"
#include "ObfusString.hpp"

NAMESPACE_SOUP
{
#if SOUP_EXCEPTIONS
	static void onException(Worker& w, const std::exception& e, Scheduler&)
	{
		std::string msg = "Exception in DetachedScheduler: ";
		msg.append(e.what());
		logWriteLine(std::move(msg));

		msg = "> Raised by ";
		msg.append(w.toString());
		logWriteLine(std::move(msg));
	}
#endif

	DetachedScheduler::DetachedScheduler(netConfig&& conf) noexcept
		: conf(std::move(conf))
	{
#if SOUP_EXCEPTIONS
		on_exception = &onException;
#endif
	}

	DetachedScheduler::~DetachedScheduler() SOUP_EXCAL
	{
		setDontMakeReusableSockets();
		closeReusableSockets();
	}

	void DetachedScheduler::addWorker(SharedPtr<Worker>&& w)
	{
		Scheduler::addWorker(std::move(w));
		if (!thrd.isRunning())
		{
			thrd.start([](Capture&& cap)
			{
				cap.get<DetachedScheduler*>()->threadFunc();
			}, this);
		}
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

		std::string toString() const SOUP_EXCAL final
		{
			return ObfusString("UpdateConfigTask").str();
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

		std::string toString() const SOUP_EXCAL final
		{
			return ObfusString("CloseReusableSocketsTask").str();
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
