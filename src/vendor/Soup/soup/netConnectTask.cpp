#include "netConnectTask.hpp"
#if !SOUP_WASM

#include "netConfig.hpp"
#include "netStatus.hpp"
#include "ObfusString.hpp"
#include "rand.hpp"
#include "Scheduler.hpp"

NAMESPACE_SOUP
{
	netConnectTask::netConnectTask(const std::string& host, uint16_t port, bool prefer_ipv6)
	{
		if (IpAddr ip; ip.fromString(host))
		{
			second_lookup = true; // No retry possible
			proceedToConnect(ip, port);
		}
		else
		{
			lookup = netConfig::get().dns_resolver->makeLookupTask(prefer_ipv6 ? DNS_AAAA : DNS_A, host);
			current_lookup_is_ipv6 = prefer_ipv6;

			// In case we get no A records, we need enough data to start AAAA query.
			this->host = host;

			// In order to connect after lookup, we need to remember the port.
			this->port = port;
		}
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
						// No DNS results, bail
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
						started_connect_at = -2; // signal for `getStatus` that it was a L4 timeout
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
				if (res == -1)
				{
					// Error
					if (!second_lookup)
					{
						doSecondLookup();
						return;
					}
					started_connect_at = -1; // signal for `getStatus` that it was a L4 error
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
		lookup = netConfig::get().dns_resolver->makeLookupTask(current_lookup_is_ipv6 ? DNS_AAAA : DNS_A, host);
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
			str.append(current_lookup_is_ipv6 ? ObfusString("AAAA") : ObfusString("A"));
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
		if (!isWorkDone())
		{
			return NET_PENDING;
		}
		if (started_connect_at == 0)
		{
			return NET_FAIL_NO_DNS_RESULTS;
		}
		if (started_connect_at == -2)
		{
			return NET_FAIL_L4_TIMEOUT;
		}
		if (started_connect_at == -1)
		{
			return NET_FAIL_L4_ERROR;
		}
		return NET_OK;
	}
}

#endif
