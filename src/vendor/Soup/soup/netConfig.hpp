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

#if !SOUP_WASM || SOUP_EMSCRIPTEN
		SharedPtr<dnsResolver> dns_resolver;

		[[nodiscard]] SharedPtr<dnsResolver>& getDnsResolver() SOUP_EXCAL;
#endif

		netConfig();
	};
}
