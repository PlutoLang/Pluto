#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include "Task.hpp"

#include "dnsLookupTask.hpp"
#include "SharedPtr.hpp"
#include "Socket.hpp"
#include "time.hpp"

NAMESPACE_SOUP
{
	class netConnectTask : public Task
	{
	protected:
		std::string host;
		UniquePtr<dnsLookupTask> lookup;
		Socket sock;
		uint16_t port;
		time_t started_connect_at = 0;
		bool current_lookup_is_ipv6 = false;
		bool second_lookup = false;
		netStatus status;

	public:
		netConnectTask(const char* host, uint16_t port, bool prefer_ipv6 = false)
			: netConnectTask(std::string(host), port, prefer_ipv6)
		{
		}

		netConnectTask(const std::string& host, uint16_t port, bool prefer_ipv6 = false);

		netConnectTask(const IpAddr& addr, uint16_t port);

		void onTick() final;

		[[nodiscard]] bool wasSuccessful() const; // Output
		[[nodiscard]] SharedPtr<Socket> getSocket(); // Output
		[[nodiscard]] SharedPtr<Socket> getSocket(Scheduler& sched); // Output

	protected:
		void doSecondLookup();
		void proceedToConnect(const IpAddr& addr, uint16_t port);

	public:
		[[nodiscard]] std::string toString() const SOUP_EXCAL final;
		[[nodiscard]] netStatus getStatus() const noexcept;
	};

	inline bool netConnectTask::wasSuccessful() const
	{
		return sock.hasConnection();
	}
}

#endif
