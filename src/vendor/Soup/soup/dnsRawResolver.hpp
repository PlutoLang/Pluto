#pragma once

#include "dnsResolver.hpp"

namespace soup
{
	struct dnsRawResolver : public dnsResolver
	{
		[[nodiscard]] static bool checkBuiltinResult(std::vector<UniquePtr<dnsRecord>>& res, dnsType qtype, const std::string& name) SOUP_EXCAL;
		[[nodiscard]] static UniquePtr<dnsLookupTask> checkBuiltinResultTask(dnsType qtype, const std::string& name) SOUP_EXCAL;

		[[nodiscard]] static std::string getQuery(dnsType qtype, const std::string& name) SOUP_EXCAL;
		[[nodiscard]] static std::vector<UniquePtr<dnsRecord>> parseResponse(std::string&& data) SOUP_EXCAL;
	};
}
