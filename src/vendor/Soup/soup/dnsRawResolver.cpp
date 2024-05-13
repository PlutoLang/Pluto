#include "dnsRawResolver.hpp"

#include "dnsHeader.hpp"
#include "dnsQuestion.hpp"
#include "dnsResource.hpp"
#include "string.hpp"
#include "StringWriter.hpp"

NAMESPACE_SOUP
{
	bool dnsRawResolver::checkBuiltinResult(std::vector<UniquePtr<dnsRecord>>& res, dnsType qtype, const std::string& name) SOUP_EXCAL
	{
		if (name == "localhost")
		{
			if (qtype == DNS_A)
			{
				res.emplace_back(soup::make_unique<dnsARecord>(name, -1, SOUP_IPV4_NWE(127, 0, 0, 1)));
			}
			else if (qtype == DNS_AAAA)
			{
				res.emplace_back(soup::make_unique<dnsAaaaRecord>(name, -1, IpAddr(0, 0, 0, 0, 0, 0, 0, 1)));
			}
			return true;
		}
		return false;
	}

	UniquePtr<dnsLookupTask> dnsRawResolver::checkBuiltinResultTask(dnsType qtype, const std::string& name) SOUP_EXCAL
	{
		std::vector<UniquePtr<dnsRecord>> res;
		if (checkBuiltinResult(res, qtype, name))
		{
			return dnsCachedResultTask::make(std::move(res));
		}
		return {};
	}

	std::string dnsRawResolver::getQuery(dnsType qtype, const std::string& name, uint16_t id) SOUP_EXCAL
	{
		StringWriter sw(false);

		dnsHeader dh{};
		dh.id = id;
		dh.setRecursionDesired(true);
		dh.qdcount = 1;
		dh.write(sw);

		dnsQuestion dq;
		dq.name.name = string::explode(name, '.');
		dq.qtype = qtype;
		dq.write(sw);

		return sw.data;
	}

	std::vector<UniquePtr<dnsRecord>> dnsRawResolver::parseResponse(std::string&& data) SOUP_EXCAL
	{
		StringReader sr(std::move(data), false);

		dnsHeader dh;
		dh.read(sr);

		for (uint16_t i = 0; i != dh.qdcount; ++i)
		{
			dnsQuestion dq;
			dq.read(sr);
		}

		std::vector<UniquePtr<dnsRecord>> res{};
		for (uint16_t i = 0; i != dh.ancount; ++i)
		{
			dnsResource dr;
			dr.read(sr);

			std::string name = string::join(dr.name.resolve(sr.data), '.');

			if (dr.rclass != DNS_IN)
			{
				continue;
			}

			if (dr.rtype == DNS_A)
			{
				res.emplace_back(soup::make_unique<dnsARecord>(std::move(name), dr.ttl, *reinterpret_cast<uint32_t*>(dr.rdata.data())));
			}
			else if (dr.rtype == DNS_AAAA)
			{
				res.emplace_back(soup::make_unique<dnsAaaaRecord>(std::move(name), dr.ttl, reinterpret_cast<uint8_t*>(dr.rdata.data())));
			}
			else if (dr.rtype == DNS_CNAME)
			{
				StringReader rdata_sr(std::move(dr.rdata), false);

				dnsName cname;
				cname.read(rdata_sr);

				res.emplace_back(soup::make_unique<dnsCnameRecord>(std::move(name), dr.ttl, string::join(cname.resolve(sr.data), '.')));
			}
			else if (dr.rtype == DNS_PTR)
			{
				StringReader rdata_sr(std::move(dr.rdata), false);

				dnsName cname;
				cname.read(rdata_sr);

				res.emplace_back(soup::make_unique<dnsPtrRecord>(std::move(name), dr.ttl, string::join(cname.resolve(sr.data), '.')));
			}
			else if (dr.rtype == DNS_TXT)
			{
				std::string data;
				for (size_t i = 0; i != dr.rdata.size(); )
				{
					auto chunk_size = (uint8_t)dr.rdata.at(i++);
					data.append(dr.rdata.substr(i, chunk_size));
					i += chunk_size;
				}
				res.emplace_back(soup::make_unique<dnsTxtRecord>(std::move(name), dr.ttl, std::move(data)));
			}
			else if (dr.rtype == DNS_MX)
			{
				uint16_t priority;
				dnsName target;

				StringReader rdata_sr(std::move(dr.rdata), false);
				rdata_sr.u16_be(priority);
				target.read(rdata_sr);

				res.emplace_back(soup::make_unique<dnsMxRecord>(std::move(name), dr.ttl, priority, string::join(target.resolve(sr.data), '.')));
			}
			else if (dr.rtype == DNS_SRV)
			{
				uint16_t priority, weight, port;
				dnsName target;

				StringReader rdata_sr(std::move(dr.rdata), false);
				rdata_sr.u16_be(priority);
				rdata_sr.u16_be(weight);
				rdata_sr.u16_be(port);
				target.read(rdata_sr);

				res.emplace_back(soup::make_unique<dnsSrvRecord>(std::move(name), dr.ttl, priority, weight, string::join(target.resolve(sr.data), '.'), port));
			}
			else if (dr.rtype == DNS_NS)
			{
				StringReader rdata_sr(std::move(dr.rdata), false);

				dnsName cname;
				cname.read(rdata_sr);

				res.emplace_back(soup::make_unique<dnsNsRecord>(std::move(name), dr.ttl, string::join(cname.resolve(sr.data), '.')));
			}
		}
		return res;
	}
}
