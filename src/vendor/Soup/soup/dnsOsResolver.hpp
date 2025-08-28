#pragma once

#include "base.hpp"
#if !SOUP_ANDROID && !SOUP_WASM
#include "dnsResolver.hpp"

NAMESPACE_SOUP
{
	struct dnsOsResolver : public dnsResolver
	{
		[[nodiscard]] Optional<std::vector<UniquePtr<dnsRecord>>> lookup(dnsType qtype, const std::string& name) const final;

		[[nodiscard]] static Optional<std::vector<UniquePtr<dnsRecord>>> staticLookup(dnsType qtype, const std::string& name);
	};
}

#endif
