#pragma once

#include <vector>

#include "dnsLookupTask.hpp"
#include "dns_records.hpp"
#include "TransientToken.hpp"
#include "UniquePtr.hpp"

namespace soup
{
	struct dnsResolver
	{
		TransientToken transient_token;

		virtual ~dnsResolver() noexcept = default;

		[[nodiscard]] std::vector<IpAddr> lookupIPv4(const std::string& name) const;
		[[nodiscard]] std::vector<IpAddr> lookupIPv6(const std::string& name) const;

		// Note that lookup may return records of differing types, e.g. (DNS_A, "au2-auto-tcp.ptoserver.com") will return CNAME record "ausd2-auto-tcp.ptoserver.com" which then gets resolved to A record "91.242.215.105", but the intermediate CNAME is still returned!
		[[nodiscard]] virtual std::vector<UniquePtr<dnsRecord>> lookup(dnsType qtype, const std::string& name) const;
		[[nodiscard]] virtual UniquePtr<dnsLookupTask> makeLookupTask(dnsType qtype, const std::string& name) const;

		[[nodiscard]] static std::vector<IpAddr> simplifyIPv4LookupResults(const std::vector<UniquePtr<dnsRecord>>& vec);
		[[nodiscard]] static std::vector<IpAddr> simplifyIPv6LookupResults(const std::vector<UniquePtr<dnsRecord>>& vec);
	};
}
