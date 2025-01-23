#pragma once

#include "dnsResolver.hpp"

NAMESPACE_SOUP
{
	struct dnsRawResolver : public dnsResolver
	{
		[[nodiscard]] static bool checkBuiltinResult(std::vector<UniquePtr<dnsRecord>>& res, dnsType qtype, const std::string& name) SOUP_EXCAL;
		[[nodiscard]] static UniquePtr<dnsLookupTask> checkBuiltinResultTask(dnsType qtype, const std::string& name) SOUP_EXCAL;

		[[nodiscard]] static std::string getQuery(dnsType qtype, const std::string& name, uint16_t id = 0) SOUP_EXCAL;
		[[nodiscard]] static std::vector<UniquePtr<dnsRecord>> parseResponse(const std::string& data) SOUP_EXCAL;
	};
}
