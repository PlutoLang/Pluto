#include "dnsOsResolver.hpp"

#if SOUP_WINDOWS || SOUP_LINUX

#if SOUP_WINDOWS
#undef UNICODE
#include "windns.h"
#pragma comment(lib, "dnsapi.lib")
#else
#include <resolv.h>

#include "dnsClass.hpp" // for DNS_IN because C_IN is not available on MacOS, not even in <arpa/nameser.h>
#endif

NAMESPACE_SOUP
{
	Optional<std::vector<UniquePtr<dnsRecord>>> dnsOsResolver::lookup(dnsType qtype, const std::string& name) const
	{
		return staticLookup(qtype, name);
	}

	Optional<std::vector<UniquePtr<dnsRecord>>> dnsOsResolver::staticLookup(dnsType qtype, const std::string& name)
	{
#if SOUP_WINDOWS
		PDNS_RECORD pDnsRecord;
		if (DnsQuery_UTF8(name.c_str(), qtype, DNS_QUERY_STANDARD, 0, &pDnsRecord, 0) == ERROR_SUCCESS)
		{
			std::vector<UniquePtr<dnsRecord>> res{};
			for (PDNS_RECORD i = pDnsRecord; i; i = i->pNext)
			{
				if (i->wType == DNS_TYPE_A)
				{
					res.emplace_back(soup::make_unique<dnsARecord>(i->pName, i->dwTtl, i->Data.A.IpAddress));
				}
				else if (i->wType == DNS_TYPE_AAAA)
				{
					res.emplace_back(soup::make_unique<dnsAaaaRecord>(i->pName, i->dwTtl, i->Data.AAAA.Ip6Address.IP6Byte));
				}
				else if (i->wType == DNS_TYPE_CNAME)
				{
					res.emplace_back(soup::make_unique<dnsCnameRecord>(i->pName, i->dwTtl, i->Data.CNAME.pNameHost));
				}
				else if (i->wType == DNS_TYPE_PTR)
				{
					res.emplace_back(soup::make_unique<dnsPtrRecord>(i->pName, i->dwTtl, i->Data.PTR.pNameHost));
				}
				else if (i->wType == DNS_TYPE_TEXT)
				{
					std::string data;
					for (DWORD j = 0; j != i->Data.TXT.dwStringCount; ++j)
					{
						data.append(i->Data.TXT.pStringArray[j]);
					}
					res.emplace_back(soup::make_unique<dnsTxtRecord>(i->pName, i->dwTtl, std::move(data)));
				}
				else if (i->wType == DNS_TYPE_MX)
				{
					res.emplace_back(soup::make_unique<dnsMxRecord>(i->pName, i->dwTtl, i->Data.MX.wPreference, i->Data.MX.pNameExchange));
				}
				else if (i->wType == DNS_TYPE_SRV)
				{
					res.emplace_back(soup::make_unique<dnsSrvRecord>(i->pName, i->dwTtl, i->Data.SRV.wPriority, i->Data.SRV.wWeight, i->Data.SRV.pNameTarget, i->Data.SRV.wPort));
				}
				else if (i->wType == DNS_TYPE_NS)
				{
					res.emplace_back(soup::make_unique<dnsNsRecord>(i->pName, i->dwTtl, i->Data.NS.pNameHost));
				}
			}
			DnsRecordListFree(pDnsRecord, DnsFreeRecordListDeep);
			return res;
		}
#elif SOUP_LINUX
		unsigned char query_buffer[1024];
		auto ret = res_query(name.c_str(), DNS_IN, qtype, query_buffer, sizeof(query_buffer));
		if (ret > 0)
		{
			std::vector<UniquePtr<dnsRecord>> res{};
			ns_msg nsMsg;
			ns_initparse(query_buffer, ret, &nsMsg);
			for (int i = 0; i < ns_msg_count(nsMsg, ns_s_an); ++i)
			{
				ns_rr rr;
				ns_parserr(&nsMsg, ns_s_an, i, &rr);
				if (ns_rr_type(rr) == ns_t_a)
				{
					res.emplace_back(soup::make_unique<dnsARecord>(ns_rr_name(rr), ns_rr_ttl(rr), *(const uint32_t*)ns_rr_rdata(rr)));
				}
				else if (ns_rr_type(rr) == ns_t_aaaa)
				{
					res.emplace_back(soup::make_unique<dnsAaaaRecord>(ns_rr_name(rr), ns_rr_ttl(rr), (const uint8_t*)ns_rr_rdata(rr)));
				}
				else if (ns_rr_type(rr) == ns_t_cname)
				{
					char hostname[1024];
					dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr), hostname, sizeof(hostname));
					res.emplace_back(soup::make_unique<dnsCnameRecord>(ns_rr_name(rr), ns_rr_ttl(rr), hostname));
				}
				else if (ns_rr_type(rr) == ns_t_ptr)
				{
					char hostname[1024];
					dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr), hostname, sizeof(hostname));
					res.emplace_back(soup::make_unique<dnsPtrRecord>(ns_rr_name(rr), ns_rr_ttl(rr), hostname));
				}
				else if (ns_rr_type(rr) == ns_t_txt)
				{
					res.emplace_back(soup::make_unique<dnsTxtRecord>(ns_rr_name(rr), ns_rr_ttl(rr), (const char*)(ns_rr_rdata(rr) + 1)));
				}
				else if (ns_rr_type(rr) == ns_t_mx)
				{
					char hostname[1024];
					dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr) + 2, hostname, sizeof(hostname));
					res.emplace_back(soup::make_unique<dnsMxRecord>(ns_rr_name(rr), ns_rr_ttl(rr), ntohs(*(unsigned short*)ns_rr_rdata(rr)), hostname));
				}
				else if (ns_rr_type(rr) == ns_t_srv)
				{
					char hostname[1024];
					dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr) + 6, hostname, sizeof(hostname));
					res.emplace_back(soup::make_unique<dnsSrvRecord>(ns_rr_name(rr), ns_rr_ttl(rr), ntohs(*(unsigned short*)ns_rr_rdata(rr)), ntohs(*((unsigned short*)ns_rr_rdata(rr) + 1)), hostname, ntohs(*((unsigned short*)ns_rr_rdata(rr) + 2))));
				}
				else if (ns_rr_type(rr) == ns_t_ns)
				{
					char hostname[1024];
					dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr), hostname, sizeof(hostname));
					res.emplace_back(soup::make_unique<dnsNsRecord>(ns_rr_name(rr), ns_rr_ttl(rr), hostname));
				}
			}
			return res;
		}
#endif
		return std::nullopt;
	}
}

#endif
