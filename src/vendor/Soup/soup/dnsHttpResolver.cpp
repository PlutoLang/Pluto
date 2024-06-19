#include "dnsHttpResolver.hpp"

#if !SOUP_WASM

#include "base64.hpp"
#include "DelayedCtor.hpp"
#include "HttpRequest.hpp"
#include "HttpRequestTask.hpp"
#include "ObfusString.hpp"

NAMESPACE_SOUP
{
	Optional<std::vector<UniquePtr<dnsRecord>>> dnsHttpResolver::lookup(dnsType qtype, const std::string& name) const
	{
		std::vector<UniquePtr<dnsRecord>> res;
		if (checkBuiltinResult(res, qtype, name))
		{
			return res;
		}

		std::string path = "/dns-query?dns=";
		path.append(base64::urlEncode(getQuery(qtype, name)));

		HttpRequest hr(std::string(server), std::move(path));
		auto hres = hr.execute(keep_alive_sched);

		return parseResponse(std::move(hres->body));
	}

	struct dnsHttpLookupTask : public dnsLookupTask
	{
		DelayedCtor<HttpRequestTask> http;

		dnsHttpLookupTask(IpAddr&& server, dnsType qtype, const std::string& name)
		{
			std::string url = ObfusString("https://");
			url.append(server.toString());
			url.append(ObfusString("/dns-query?dns=").str());
			url.append(base64::urlEncode(dnsRawResolver::getQuery(qtype, name)));

			http.construct(Uri(url));
		}

		void onTick() final
		{
			if (http->tickUntilDone())
			{
				if (http->result)
				{
					result = dnsRawResolver::parseResponse(std::move(http->result->body));
				}
				setWorkDone();
			}
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL final
		{
			std::string str = ObfusString("dnsHttpLookupTask");
			str.append(": ");
			str.push_back('[');
			str.append(http->toString());
			str.push_back(']');
			return str;
		}
	};

	UniquePtr<dnsLookupTask> dnsHttpResolver::makeLookupTask(dnsType qtype, const std::string& name) const
	{
		if (auto res = checkBuiltinResultTask(qtype, name))
		{
			return res;
		}
		IpAddr server;
		SOUP_ASSERT(server.fromString(this->server));
		return soup::make_unique<dnsHttpLookupTask>(std::move(server), qtype, name);
	}
}

#endif
