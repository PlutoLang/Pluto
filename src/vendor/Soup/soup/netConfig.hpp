#pragma once

#include "base.hpp"
#include "type.hpp"

#include "dnsResolver.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct netConfig
	{
		[[nodiscard]] static netConfig& get(); // returns the netConfig instance for this thread

#if !SOUP_WASM
		int connect_timeout_ms = 3000;
#endif
		UniquePtr<dnsResolver> dns_resolver;
#if !SOUP_WASM
		certchain_validator_t certchain_validator;
#endif

		[[nodiscard]] dnsResolver& getDnsResolver() SOUP_EXCAL;

		netConfig();
	};
}
