#include "Scheduler.hpp"

#if !SOUP_WINDOWS
#include <netinet/tcp.h> // TCP_NODELAY
#endif

#include "log.hpp"
#include "os.hpp"
#include "Promise.hpp"
#include "ReuseTag.hpp"
#include "Socket.hpp"
#include "Task.hpp"
#include "time.hpp"

#define LOG_TICK_DUR false

#if LOG_TICK_DUR
#include "format.hpp"
#include "Stopwatch.hpp"
#endif

NAMESPACE_SOUP
{
	void Scheduler::addWorker(SharedPtr<Worker>&& w) SOUP_EXCAL
	{
		pending_workers.emplace_front(std::move(w));
	}

#if !SOUP_WASM
	SharedPtr<Socket> Scheduler::addSocket() SOUP_EXCAL
	{
		auto s = soup::make_shared<Socket>();
		addSocket(s);
		return s;
	}

	void Scheduler::addSocket(SharedPtr<Socket> sock) SOUP_EXCAL
	{
#if !SOUP_WINDOWS
		sock->setNonBlocking();
#endif
		sock->setOpt<int>(IPPROTO_TCP, TCP_NODELAY, 1);
		return addWorker(std::move(sock));
	}
#endif

	void Scheduler::run()
	{
		const auto prev_scheduler = this_thread_running_scheduler;
		this_thread_running_scheduler = this;
		std::vector<pollfd> pollfds{};
		while (shouldKeepRunning())
		{
			uint8_t workload_flags = 0;
#if LOG_TICK_DUR
			Stopwatch t;
#endif
			tick(pollfds, workload_flags);
#if LOG_TICK_DUR
			t.stop();
			logWriteLine(format("Tick took {} ms", t.getMs()));
#endif
			if (workload_flags & NOT_JUST_SOCKETS)
			{
				yieldBusyspin(pollfds, workload_flags);
			}
			else
			{
				yieldKernel(pollfds);
			}
			pollfds.clear();
		}
		this_thread_running_scheduler = prev_scheduler;
	}

	void Scheduler::runFor(unsigned int ms)
	{
		const auto prev_scheduler = this_thread_running_scheduler;
		this_thread_running_scheduler = this;
		time_t deadline = time::millis() + ms;
		std::vector<pollfd> pollfds{};
		while (shouldKeepRunning())
		{
			uint8_t workload_flags = 0;
			tick(pollfds, workload_flags);
			yieldBusyspin(pollfds, workload_flags);
			if (time::millis() > deadline)
			{
				break;
			}
			pollfds.clear();
		}
		this_thread_running_scheduler = prev_scheduler;
	}

	bool Scheduler::shouldKeepRunning() const noexcept
	{
		return workers.size() != passive_workers || !pending_workers.empty();
	}

	void Scheduler::tick()
	{
		const auto prev_scheduler = this_thread_running_scheduler;
		this_thread_running_scheduler = this;

		std::vector<pollfd> pollfds{};
		uint8_t workload_flags = 0;
		tick(pollfds, workload_flags);
#if !SOUP_WASM
		if (poll(pollfds, 0) > 0)
		{
			processPollResults(pollfds);
		}
#endif

		this_thread_running_scheduler = prev_scheduler;
	}

	void Scheduler::tick(std::vector<pollfd>& pollfds, uint8_t& workload_flags)
	{
		// Schedule in pending workers
		{
			auto num_pending_workers = pending_workers.size();
			SOUP_IF_UNLIKELY (num_pending_workers != 0)
			{
				workers.reserve(workers.size() + num_pending_workers);
				do
				{
					auto worker = pending_workers.pop_back();
					workers.emplace_back(std::move(*worker));
				} while (--num_pending_workers);
			}
		}

		// Process workers
#if !SOUP_WASM
		pollfds.reserve(workers.size());
#endif
		for (auto i = workers.begin(); i != workers.end(); )
		{
			if ((*i)->type == WORKER_TYPE_SOCKET)
			{
#if !SOUP_WASM
				SOUP_IF_UNLIKELY (static_cast<Socket*>(i->get())->fd == -1)
				{
					processClosedSocket(*static_cast<Socket*>(i->get()));
				}
#endif
			}
			SOUP_IF_UNLIKELY ((*i)->holdup_type == Worker::NONE)
			{
				if (on_work_done)
				{
					on_work_done(*i->get(), *this);
				}
				i = workers.erase(i);
				continue;
			}
			tickWorker(pollfds, workload_flags, **i);
			++i;
		}
	}

	void Scheduler::tickWorker(std::vector<pollfd>& pollfds, uint8_t& workload_flags, Worker& w)
	{
#if !SOUP_WASM
		if (w.holdup_type == Worker::SOCKET)
		{
			pollfds.emplace_back(pollfd{
				static_cast<Socket&>(w).fd,
				POLLIN
			});
		}
		else
#endif
		{
#if !SOUP_WASM
			pollfds.emplace_back(pollfd{
				(Socket::fd_t)-1,
				0
			});
#endif

			int dispo = Worker::NEUTRAL;

			if (w.holdup_type == Worker::IDLE)
			{
				if (w.type == WORKER_TYPE_TASK)
				{
					dispo = static_cast<Task&>(w).getSchedulingDisposition();
				}
				fireHoldupCallback(w);
			}
			else if (w.holdup_type == Worker::PROMISE_BASE)
			{
				if (!reinterpret_cast<PromiseBase*>(w.holdup_data)->isPending())
				{
					fireHoldupCallback(w);
				}
			}
			else //if (w.holdup_type == Worker::PROMISE_VOID)
			{
				if (!reinterpret_cast<Promise<void>*>(w.holdup_data)->isPending())
				{
					fireHoldupCallback(w);
				}
			}

			workload_flags |= dispo;
			static_assert((int)Worker::HIGH_FRQUENCY == ((int)HAS_HIGH_FREQUENCY_TASKS | (int)NOT_JUST_SOCKETS));
			static_assert((int)Worker::NEUTRAL == (int)NOT_JUST_SOCKETS);
			static_assert((int)Worker::LOW_FREQUENCY == (int)0);
		}
	}

	void Scheduler::yieldBusyspin(std::vector<pollfd>& pollfds, uint8_t workload_flags)
	{
#if !SOUP_WASM
		if (poll(pollfds, 0) > 0)
		{
			processPollResults(pollfds);
		}
#endif
		if (!(workload_flags & HAS_HIGH_FREQUENCY_TASKS))
		{
			os::sleep(1);
		}
	}

	void Scheduler::yieldKernel(std::vector<pollfd>& pollfds)
	{
#if !SOUP_WASM
		// Why not timeout=-1?
		// - On Linux, poll does not detect closed sockets, even if shutdown is used.
		// - If a scheduler that is only waiting on sockets has addWorker called on it, we don't want an insane delay until that worker starts.
		int timeout = 50;
#if SOUP_WINDOWS
		if (add_worker_can_wait_forever_for_all_i_care)
		{
			timeout = -1;
		}
#endif
		if (poll(pollfds, timeout) > 0)
		{
			processPollResults(pollfds);
		}
#endif
	}

#if !SOUP_WASM
	int Scheduler::poll(std::vector<pollfd>& pollfds, int timeout)
	{
#if SOUP_WINDOWS
		return ::WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), timeout);
#else
		return ::poll(pollfds.data(), pollfds.size(), timeout);
#endif
	}

	void Scheduler::processPollResults(const std::vector<pollfd>& pollfds)
	{
		for (auto i = pollfds.begin(); i != pollfds.end(); ++i)
		{
			if (i->revents != 0
				&& i->fd != -1
				)
			{
				auto workers_i = workers.begin() + (i - pollfds.begin());
				if (i->revents & ~POLLIN)
				{
					static_cast<Socket*>(workers_i->get())->remote_closed = true;
					processClosedSocket(*static_cast<Socket*>(workers_i->get()));
				}
				else
				{
					fireHoldupCallback(**workers_i);
				}
			}
		}
	}
#endif

	void Scheduler::fireHoldupCallback(Worker& w)
	{
#if defined(_DEBUG) || !SOUP_EXCEPTIONS
		w.fireHoldupCallback();
#else
		try
		{
			w.fireHoldupCallback();
		}
		catch (const std::exception& e)
		{
			if (on_exception)
			{
				on_exception(w, e, *this);
			}
			w.holdup_type = Worker::NONE;
		}
#endif
	}

#if !SOUP_WASM
	void Scheduler::processClosedSocket(Socket& s)
	{
		if (on_connection_lost)
		{
			if (!s.dispatched_connection_lost)
			{
				s.dispatched_connection_lost = true;
				on_connection_lost(s, *this);
			}
		}

		if (s.holdup_type == Worker::SOCKET)
		{
			if (s.callback_recv_on_close
				|| s.transport_hasData()
				)
			{
				// Socket still has stuff to do...
				fireHoldupCallback(s);
			}
			else
			{
				// No excuses, slate for execution.
				s.holdup_type = Worker::NONE;
			}
		}
	}
#endif

	size_t Scheduler::getNumWorkers() const
	{
		return workers.size();
	}

	size_t Scheduler::getNumWorkersOfType(uint8_t type) const
	{
		size_t res = 0;
		for (const auto& w : workers)
		{
			if (w->type == type)
			{
				++res;
			}
		}
		return res;
	}

	size_t Scheduler::getNumSockets() const
	{
		return getNumWorkersOfType(WORKER_TYPE_SOCKET);
	}

	SharedPtr<Worker> Scheduler::getShared(const Worker& w) const
	{
		for (const auto& spW : workers)
		{
			if (spW.get() == &w)
			{
				return spW;
			}
		}
		return {};
	}

#if !SOUP_WASM
	SharedPtr<Socket> Scheduler::findReusableSocket(const std::string& host, uint16_t port, bool tls)
	{
		for (const auto& w : workers)
		{
			if (w->type == WORKER_TYPE_SOCKET
				&& static_cast<Socket*>(w.get())->custom_data.isStructInMap(ReuseTag)
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).host == host
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).port == port
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).tls == tls
				)
			{
				return w;
			}
		}

		// Iterating over the AtomicDeque is fine here because this function should only be called on the scheduler thread, which is the same one that would pop.
		for (auto node = pending_workers.head.load(); node != nullptr; node = node->next.load())
		{
			const SharedPtr<Socket>& w = node->data;
			if (w->type == WORKER_TYPE_SOCKET
				&& static_cast<Socket*>(w.get())->custom_data.isStructInMap(ReuseTag)
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).host == host
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).port == port
				&& static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).tls == tls
				)
			{
				return w;
			}
		}

		return {};
	}

	void Scheduler::closeReusableSockets() SOUP_EXCAL
	{
		for (const auto& w : workers)
		{
			if (w->type == WORKER_TYPE_SOCKET
				&& static_cast<Socket*>(w.get())->custom_data.isStructInMap(ReuseTag)
				&& !static_cast<Socket*>(w.get())->custom_data.getStructFromMapConst(ReuseTag).is_busy
				)
			{
				static_cast<Socket*>(w.get())->close();
			}
		}
	}
#endif

	void Scheduler::on_exception_log(Worker& w, const std::exception& e, Scheduler&)
	{
		std::string msg = "Exception while processing ";
		msg.append(w.toString());
		msg.append(": ");
		msg.append(e.what());
		logWriteLine(std::move(msg));
	}
}
