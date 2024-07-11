#include "netConnectTask.hpp"
#if !SOUP_WASM

#if !SOUP_WINDOWS
#include <netinet/tcp.h> // TCP_NODELAY
#endif

#include "netConfig.hpp"
#include "netStatus.hpp"
#include "ObfusString.hpp"
#include "rand.hpp"
#include "Scheduler.hpp"

NAMESPACE_SOUP
{
	netConnectTask::netConnectTask(const std::string& host, uint16_t port, bool prefer_ipv6)
		: status(NET_PENDING)
	{
		if (IpAddr ip; ip.fromString(host))
		{
			second_lookup = true; // No retry possible
			proceedToConnect(ip, port);
		}
		else
		{
			lookup = netConfig::get().getDnsResolver().makeLookupTask(prefer_ipv6 ? DNS_AAAA : DNS_A, host);
			current_lookup_is_ipv6 = prefer_ipv6;

			// In case we get no A records, we need enough data to start AAAA query.
			this->host = host;

			// In order to connect after lookup, we need to remember the port.
			this->port = port;
		}
	}

	netConnectTask::netConnectTask(const IpAddr& addr, uint16_t port)
		: status(NET_PENDING)
	{
		proceedToConnect(addr, port);
	}

	void netConnectTask::onTick()
	{
		if (lookup)
		{
			if (lookup->tickUntilDone())
			{
				std::vector<IpAddr> results{};
				if (current_lookup_is_ipv6)
				{
					results = dnsResolver::simplifyIPv6LookupResults(lookup->result);
				}
				else
				{
					results = dnsResolver::simplifyIPv4LookupResults(lookup->result);
				}

				if (results.empty())
				{
					if (second_lookup)
					{
						if (lookup->result.has_value())
						{
							status = NET_FAIL_NO_DNS_RESPONSE;
						}
						else
						{
							status = NET_FAIL_NO_DNS_RESULTS;
						}
						lookup.reset();
						setWorkDone();
					}
					else
					{
						doSecondLookup();
					}
				}
				else
				{
					lookup.reset();
					proceedToConnect(rand(results), port);
				}
			}
		}
		else
		{
			SOUP_ASSERT(sock.hasConnection());

			pollfd pfd;
			pfd.fd = sock.fd;
			pfd.events = POLLOUT;
			pfd.revents = 0;
#if SOUP_WINDOWS
			int res = ::WSAPoll(&pfd, 1, 0);
#else
			int res = ::poll(&pfd, 1, 0);
#endif
			if (res == 0)
			{
				// Pending
				if (time::millisSince(started_connect_at) > netConfig::get().connect_timeout_ms)
				{
					// Timeout
					sock.transport_close();
					if (second_lookup)
					{
						status = NET_FAIL_L4_TIMEOUT;
						setWorkDone();
					}
					else
					{
						doSecondLookup();
					}
				}
			}
			else
			{
				if (res != -1)
				{
					// Success
					sock.setOpt<int>(IPPROTO_TCP, TCP_NODELAY, 1);
					status = NET_OK;
				}
				else
				{
					// Error
					if (!second_lookup)
					{
						doSecondLookup();
						return;
					}
					status = NET_FAIL_L4_ERROR;
					sock.transport_close();
				}
				setWorkDone();
			}
		}
	}

	SharedPtr<Socket> netConnectTask::getSocket()
	{
		return getSocket(*Scheduler::get());
	}

	SharedPtr<Socket> netConnectTask::getSocket(Scheduler& sched)
	{
		SOUP_ASSERT(sock.hasConnection());
		return sched.addSocket(std::move(this->sock));
	}

	void netConnectTask::doSecondLookup()
	{
		current_lookup_is_ipv6 = !current_lookup_is_ipv6;
		lookup = netConfig::get().getDnsResolver().makeLookupTask(current_lookup_is_ipv6 ? DNS_AAAA : DNS_A, host);
		second_lookup = true;
	}

	void netConnectTask::proceedToConnect(const IpAddr& addr, uint16_t port)
	{
		SOUP_ASSERT(sock.kickOffConnect(addr, port));
		started_connect_at = time::millis();
	}

	std::string netConnectTask::toString() const SOUP_EXCAL
	{
		std::string str = ObfusString("netConnectTask");
		str.append(": ");
		if (lookup)
		{
			str.append(ObfusString("Lookup #").str());
			str.push_back(second_lookup ? '2' : '1');
			str.append(": ");
			str.append(current_lookup_is_ipv6 ? ObfusString("AAAA").str() : ObfusString("A").str());
			str.append(": ");
			str.push_back('[');
			str.append(lookup->toString());
			str.push_back(']');
		}
		else
		{
			str.append(ObfusString("Handshaking").str());
		}
		return str;
	}

	netStatus netConnectTask::getStatus() const noexcept
	{
		return status;
	}
}

#endif
