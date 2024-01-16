#include "dnsSmartResolver.hpp"
#if !SOUP_WASM

#include "dnsHttpResolver.hpp"
#include "dnsLookupTask.hpp"
#include "dnsUdpResolver.hpp"
#include "ObfusString.hpp"
#include "WeakRef.hpp"

namespace soup
{
	struct dnsSmartLookupTask : public dnsLookupTask
	{
		WeakRef<const dnsSmartResolver> resolv_wr;
		dnsType qtype;
		std::string name;

		UniquePtr<dnsLookupTask> subtask;
		UniquePtr<dnsHttpResolver> http_resolver;

		dnsSmartLookupTask(const dnsSmartResolver& resolv, dnsType qtype, const std::string& name)
			: resolv_wr(&resolv), qtype(qtype), name(name)
		{
		}

		void onTick() final
		{
			if (!subtask)
			{
				if (auto resolv = resolv_wr.getPointer())
				{
					subtask = resolv->subresolver->makeLookupTask(qtype, name);
				}
			}
			else
			{
				if (subtask->tickUntilDone())
				{
					if (auto resolv = resolv_wr.getPointer())
					{
						if (!resolv->tested_udp)
						{
							if (!http_resolver)
							{
								// This was a UDP query
								if (!subtask->result.empty())
								{
									resolv->tested_udp = true; // Yup, UDP works!
									fulfil(std::move(subtask->result));
								}
								else
								{
									std::string server = static_cast<dnsUdpResolver*>(resolv->subresolver.get())->server.ip.toString();
									http_resolver = soup::make_unique<dnsHttpResolver>();
									static_cast<dnsHttpResolver*>(http_resolver.get())->server = std::move(server);
									subtask = http_resolver->makeLookupTask(qtype, name);
								}
							}
							else
							{
								// This was an HTTP query after a previous UDP query yielded no results
								if (!subtask->result.empty())
								{
									resolv->subresolver = std::move(http_resolver);
									resolv->tested_udp = true; // UDP does not work, but at least we tested it.
									fulfil(std::move(subtask->result));
								}
								else
								{
									// There is a difference between an empty result and no result, but the APIs currently don't expose it.
									// So, we can't say what works and what doesn't at this point.
									fulfil(std::move(subtask->result));
								}
							}
						}
						else
						{
							fulfil(std::move(subtask->result));
						}
					}
					else
					{
						fulfil(std::move(subtask->result));
					}
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
		}
		return soup::make_unique<dnsSmartLookupTask>(*this, qtype, name);
	}
}

#endif
