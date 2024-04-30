#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include "type.hpp"

#include "dnsResolver.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct netConfig
	{
		[[nodiscard]] static netConfig& get(); // returns the netConfig instance for this thread

		int connect_timeout_ms = 3000;
		UniquePtr<dnsResolver> dns_resolver;
		certchain_validator_t certchain_validator;

		[[nodiscard]] dnsResolver& getDnsResolver() SOUP_EXCAL;

		netConfig();
	};
}

#endif
