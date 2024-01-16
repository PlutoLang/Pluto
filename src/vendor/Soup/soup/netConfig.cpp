#include "netConfig.hpp"
#if !SOUP_WASM

#include "dnsSmartResolver.hpp"
#include "Socket.hpp"

namespace soup
{
	static thread_local netConfig inst;

	netConfig& netConfig::get()
	{
		return inst;
	}

	netConfig::netConfig() :
		// Reasons for not defaulting to dnsOsResolver:
		// - Android doesn't have libresolv
		// - Many ISPs provide disingenuous DNS servers, even blocking sites like pastebin.com
		dns_resolver(soup::make_unique<dnsSmartResolver>()),
		certchain_validator(&Socket::certchain_validator_relaxed)
	{
	}
}

#endif
