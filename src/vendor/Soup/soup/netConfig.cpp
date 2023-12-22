#include "netConfig.hpp"

#if !SOUP_WASM

#if !SOUP_ANDROID
#include "dnsOsResolver.hpp"
#else
#include "dnsHttpResolver.hpp"
#endif
#include "Socket.hpp"

namespace soup
{
	static thread_local netConfig inst;

	netConfig& netConfig::get()
	{
		return inst;
	}

	netConfig::netConfig() :
#if !SOUP_ANDROID
		dns_resolver(soup::make_unique<dnsOsResolver>()),
#else
		dns_resolver(soup::make_unique<dnsHttpResolver>()),
#endif
		certchain_validator(&Socket::certchain_validator_relaxed)
	{
	}
}

#endif
