#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include "dnsRawResolver.hpp"

#include "SocketAddr.hpp"

NAMESPACE_SOUP
{
	struct dnsUdpResolver : public dnsRawResolver
	{
		SocketAddr server{ IpAddr(SOUP_IPV4(1, 1, 1, 1)), native_u16_t(53) };
		unsigned int timeout_ms = 200; // normally would set this to like 3000 but 1.1.1.1 responds in <50 ms and we really don't wanna wait eons
		unsigned int num_retries = 1;

		[[nodiscard]] Optional<std::vector<UniquePtr<dnsRecord>>> lookup(dnsType qtype, const std::string& name) const final;
	};
}

#endif
