#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include "Task.hpp"

#include "dnsLookupTask.hpp"
#include "dnsResolver.hpp"
#include "SharedPtr.hpp"
#include "Socket.hpp"
#include "time.hpp"

NAMESPACE_SOUP
{
	class netConnectTask : public Task
	{
	protected:
		SharedPtr<dnsResolver> resolver;
		UniquePtr<dnsLookupTask> lookup;
		std::string host;
		uint16_t port;
		bool current_lookup_is_ipv6 = false;
		netStatus status;
		unsigned int timeout_ms = 3000;
		time_t started_connect_at = 0;
		Socket sock;

	public:
		// Resolves the hostname and tries to connect to one of the listed IPv4/IPv6 addresses, trying the IPv6/IPv4 if that fails.
		// 'timeout_ms' defines how long each connection attempt can maximally take and does not include DNS lookup time.
		netConnectTask(const std::string& host, uint16_t port, bool prefer_ipv6 = false, unsigned int timeout_ms = 3000);
		netConnectTask(SharedPtr<dnsResolver> resolver, const std::string& host, uint16_t port, bool prefer_ipv6 = false, unsigned int timeout_ms = 3000);

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
