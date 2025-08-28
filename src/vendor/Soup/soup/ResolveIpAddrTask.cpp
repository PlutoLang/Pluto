#include "ResolveIpAddrTask.hpp"

#include "dnsResolver.hpp"
#include "netConfig.hpp"
#include "rand.hpp"

NAMESPACE_SOUP
{
	ResolveIpAddrTask::ResolveIpAddrTask(std::string _name)
		: name(std::move(_name))
	{
		lookup = netConfig::get().getDnsResolver()->makeLookupTask(DNS_A, name);
	}

	void ResolveIpAddrTask::onTick()
	{
		if (lookup->tickUntilDone())
		{
			if (second_lookup)
			{
				auto results = dnsResolver::simplifyIPv6LookupResults(lookup->result);
				if (results.empty())
				{
					setWorkDone();
				}
				else
				{
					fulfil(rand(results));
				}
			}
			else
			{
				auto results = dnsResolver::simplifyIPv4LookupResults(lookup->result);
				if (results.empty())
				{
					lookup = netConfig::get().getDnsResolver()->makeLookupTask(DNS_AAAA, name);
					second_lookup = true;
				}
				else
				{
					fulfil(rand(results));
				}
			}
		}
	}
}
