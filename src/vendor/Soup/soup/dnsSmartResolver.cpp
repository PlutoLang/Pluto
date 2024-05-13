#include "dnsSmartResolver.hpp"
#if !SOUP_WASM

#include "dnsHttpResolver.hpp"
#include "dnsLookupTask.hpp"
#include "dnsUdpResolver.hpp"
#include "ObfusString.hpp"
#include "WeakRef.hpp"

NAMESPACE_SOUP
{
	struct dnsSmartLookupTask : public dnsLookupTask
	{
		WeakRef<const dnsSmartResolver> resolv_wr;
		bool retry = false;
		dnsType qtype;
		std::string name;
		UniquePtr<dnsLookupTask> subtask;
		UniquePtr<dnsHttpResolver> http_resolver;

		dnsSmartLookupTask(const dnsSmartResolver& resolv, dnsType qtype, const std::string& name)
			: resolv_wr(&resolv), qtype(qtype), name(name), subtask(resolv.subresolver->makeLookupTask(qtype, name))
		{
		}

		void onTick() final
		{
			if (subtask->tickUntilDone())
			{
				if (auto resolv = resolv_wr.getPointer())
				{
					if (subtask->result.has_value())
					{
						if (retry)
						{
							resolv->subresolver = std::move(http_resolver);
							resolv->switched_to_http = true;
						}
						else if (!resolv->switched_to_http)
						{
							static_cast<dnsUdpResolver*>(resolv->subresolver.get())->num_retries = 1; // UDP resolver may now retry once to account for packet loss.
						}
						fulfil(std::move(subtask->result));
					}
					else
					{
						// Lookup failed
						if (!resolv->switched_to_http && !retry)
						{
							// Try switching to using HTTP transport
							retry = true;
							std::string server = resolv->server.toString();
							http_resolver = soup::make_unique<dnsHttpResolver>();
							static_cast<dnsHttpResolver*>(http_resolver.get())->server = std::move(server);
							subtask = http_resolver->makeLookupTask(qtype, name);
						}
						else
						{
							// Out of options, have to report failure.
							fulfil(std::move(subtask->result));
						}
					}
				}
				else
				{
					fulfil(std::move(subtask->result));
				}
			}
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL final
		{
			std::string str = ObfusString("dnsSmartLookupTask");
			if (subtask)
			{
				str.append(": ");
				str.push_back('[');
				str.append(subtask->toString());
				str.push_back(']');
			}
			return str;
		}
	};

	UniquePtr<dnsLookupTask> dnsSmartResolver::makeLookupTask(dnsType qtype, const std::string& name) const
	{
		if (!subresolver)
		{
			subresolver = soup::make_unique<dnsUdpResolver>();
			static_cast<dnsUdpResolver*>(subresolver.get())->server.ip = this->server;
			static_cast<dnsUdpResolver*>(subresolver.get())->num_retries = 0; // No retries during testing phase.
		}
		return soup::make_unique<dnsSmartLookupTask>(*this, qtype, name);
	}
}

#endif
