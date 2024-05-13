#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include "dnsResolver.hpp"

NAMESPACE_SOUP
{
	struct dnsSmartResolver : public dnsResolver
	{
		IpAddr server = SOUP_IPV4(1, 1, 1, 1);

		[[nodiscard]] UniquePtr<dnsLookupTask> makeLookupTask(dnsType qtype, const std::string& name) const final;

		mutable UniquePtr<dnsResolver> subresolver{};
		mutable bool switched_to_http = false;
	};
}

#endif
