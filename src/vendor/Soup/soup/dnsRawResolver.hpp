#pragma once

#include "dnsResolver.hpp"

namespace soup
{
	struct dnsRawResolver : public dnsResolver
	{
		[[nodiscard]] static bool checkBuiltinResult(std::vector<UniquePtr<dnsRecord>>& res, dnsType qtype, const std::string& name);
		[[nodiscard]] static UniquePtr<dnsLookupTask> checkBuiltinResultTask(dnsType qtype, const std::string& name);

		[[nodiscard]] static std::string getQuery(dnsType qtype, const std::string& name);
		[[nodiscard]] static std::vector<UniquePtr<dnsRecord>> parseResponse(std::string&& data);
	};
}
