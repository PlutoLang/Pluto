#pragma once

#include "base.hpp"
#include "type.hpp"

#include "dnsResolver.hpp"
#include "SharedPtr.hpp"

NAMESPACE_SOUP
{
	struct netConfig
	{
		[[nodiscard]] static netConfig& get(); // returns the netConfig instance for this thread

		SharedPtr<dnsResolver> dns_resolver;

		[[nodiscard]] SharedPtr<dnsResolver>& getDnsResolver() SOUP_EXCAL;

		netConfig();
	};
}
