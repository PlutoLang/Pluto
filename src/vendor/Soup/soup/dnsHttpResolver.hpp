#pragma once

#include "dnsRawResolver.hpp"

NAMESPACE_SOUP
{
	struct dnsHttpResolver : public dnsRawResolver
	{
		std::string server = "1.1.1.1";
		Scheduler* keep_alive_sched = nullptr;

		[[nodiscard]] Optional<std::vector<UniquePtr<dnsRecord>>> lookup(dnsType qtype, const std::string& name) const final;
		[[nodiscard]] UniquePtr<dnsLookupTask> makeLookupTask(dnsType qtype, const std::string& name) const final;
	};
}
