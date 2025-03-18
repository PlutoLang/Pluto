#pragma once

#include <string>

#include "dnsLookupTask.hpp"
#include "IpAddr.hpp"
#include "Optional.hpp"
#include "Task.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct ResolveIpAddrTask : public PromiseTask<Optional<IpAddr>>
	{
		std::string name;
		UniquePtr<dnsLookupTask> lookup;
		bool second_lookup = false;

		ResolveIpAddrTask(std::string _name);

		void onTick() final;
	};
}
