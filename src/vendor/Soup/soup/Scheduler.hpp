#pragma once

#include "base.hpp"

#include <string>
#include <unordered_set>
#include <vector>

#if SOUP_WINDOWS
#include <winsock2.h>
#else
#include <poll.h>
#endif

#include "AtomicDeque.hpp"
#include "SharedPtr.hpp"
#include "Worker.hpp"

NAMESPACE_SOUP
{
	// The only thing you are allowed to do from other threads is add a worker since that can be done atomically.
	// All other operations are subject to race conditions.
	class Scheduler
	{
	private:
		inline static thread_local Scheduler* this_thread_running_scheduler = nullptr;

	public:
		std::vector<SharedPtr<Worker>> workers{};
		AtomicDeque<SharedPtr<Worker>> pending_workers{};
		size_t passive_workers = 0;
		uint8_t default_workload_flags = 0;
#if !SOUP_WASM
		bool dont_make_reusable_sockets = false;
#endif
	private:
#if SOUP_WINDOWS
		bool add_worker_can_wait_forever_for_all_i_care = false;
#endif

	public:
		using on_work_done_t = void(*)(Worker&, Scheduler&);
		using on_connection_lost_t = void(*)(Socket&, Scheduler&);
#if SOUP_EXCEPTIONS
		using on_exception_t = void(*)(Worker&, const std::exception&, Scheduler&);
#endif

		on_work_done_t on_work_done = nullptr; // This will always be called before a worker is deleted.
		on_connection_lost_t on_connection_lost = nullptr; // Check remote_closed for which side caused connection to be lost.
#if SOUP_EXCEPTIONS
		on_exception_t on_exception = &on_exception_log;
#endif

		virtual ~Scheduler() = default;

		virtual void addWorker(SharedPtr<Worker>&& w);

#if !SOUP_WASM
		SharedPtr<Socket> addSocket() SOUP_EXCAL;
		void addSocket(SharedPtr<Socket> sock) SOUP_EXCAL;

		template <typename T, SOUP_RESTRICT(std::is_same_v<T, Socket>)>
		SharedPtr<Socket> addSocket(T&& sock) SOUP_EXCAL
		{
			auto s = soup::make_shared<Socket>(std::move(sock));
			addSocket(s);
			return s;
		}
#endif

		template <typename T, typename...Args>
		SharedPtr<T> add(Args&&...args) SOUP_EXCAL
		{
			auto w = soup::make_shared<T>(std::forward<Args>(args)...);
			addWorker(SharedPtr<T>(w));
			return w;
		}

		void setDontMakeReusableSockets() noexcept
		{
#if !SOUP_WASM
			dont_make_reusable_sockets = true;
#endif
		}

		void setAddWorkerCanWaitForeverForAllICare() noexcept
		{
#if SOUP_WINDOWS
			add_worker_can_wait_forever_for_all_i_care = true;
#endif
		}

		// When a scheduler only has sockets, addWorker can take up to 50ms.
		// If this is called, that delay is reduced to 1ms.
		void reduceAddWorkerDelay() noexcept
		{
			default_workload_flags |= NOT_JUST_SOCKETS;
		}

		void setHighFrequency() noexcept
		{
			default_workload_flags |= HAS_HIGH_FREQUENCY_TASKS;
		}

		void run();
		void runFor(unsigned int ms);
		[[nodiscard]] bool shouldKeepRunning() const noexcept;
		void tick();
	protected:
		enum WorkloadFlags : uint8_t
		{
			NOT_JUST_SOCKETS = 1 << 0,
			HAS_HIGH_FREQUENCY_TASKS = 1 << 1,
		};

		void tick(std::vector<pollfd>& pollfds, uint8_t& workload_flags);
		void tickWorker(std::vector<pollfd>& pollfds, uint8_t& workload_flags, Worker& w);
		void yieldBusyspin(std::vector<pollfd>& pollfds, uint8_t workload_flags);
		void yieldKernel(std::vector<pollfd>& pollfds);
#if !SOUP_WASM
		int poll(std::vector<pollfd>& pollfds, int timeout);
		void processPollResults(const std::vector<pollfd>& pollfds);
#endif
		void fireHoldupCallback(Worker& w);
#if !SOUP_WASM
		void processClosedSocket(Socket& s);
#endif

	public:
		[[nodiscard]] static Scheduler* get() { return this_thread_running_scheduler; }

		[[nodiscard]] size_t getNumWorkers() const;
		[[nodiscard]] size_t getNumWorkersOfType(uint8_t type) const;
		[[nodiscard]] size_t getNumSockets() const;

		[[nodiscard]] SharedPtr<Worker> getShared(const Worker& w) const;
#if !SOUP_WASM
		[[nodiscard]] SharedPtr<Socket> findReusableSocket(const std::string& host, uint16_t port, bool tls);
		void closeReusableSockets() SOUP_EXCAL;
#endif

		static void on_exception_log(Worker& w, const std::exception& e, Scheduler&);
	};
}
