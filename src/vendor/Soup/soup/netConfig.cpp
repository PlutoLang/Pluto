#include "netConfig.hpp"

NAMESPACE_SOUP
{
	static thread_local netConfig netConfig_inst;

	netConfig& netConfig::get()
	{
		return netConfig_inst;
	}

#if !SOUP_WASM || SOUP_EMSCRIPTEN
	SharedPtr<dnsResolver>& netConfig::getDnsResolver() SOUP_EXCAL
	{
		if (!dns_resolver)
		{
			dns_resolver = dnsResolver::makeDefault();
		}
		return dns_resolver;
	}
#endif

	netConfig::netConfig()
	{
	}
}
