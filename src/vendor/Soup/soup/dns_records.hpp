#pragma once

#include <cstdint>
#include <string>

#include "dnsType.hpp"
#include "IpAddr.hpp"

NAMESPACE_SOUP
{
	struct dnsRecord;

	using dnsRecordFactory = UniquePtr<dnsRecord>(*)(std::string&& name, uint32_t ttl, std::string&& str);

	struct dnsRecord
	{
		const dnsType type;
		std::string name;
		uint32_t ttl;

		dnsRecord(dnsType type, std::string&& name, uint32_t ttl) noexcept
			: type(type), name(std::move(name)), ttl(ttl)
		{
		}

		virtual ~dnsRecord() = default;

		[[nodiscard]] static dnsRecordFactory getFactory(dnsType type);

		[[nodiscard]] static UniquePtr<dnsRecord> fromString(dnsType type, std::string&& name, uint32_t ttl, std::string&& str);

		[[nodiscard]] virtual std::string toString() const = 0;
		[[nodiscard]] virtual std::string toRdata() const = 0;

		[[nodiscard]] UniquePtr<dnsRecord> copy() const;
	};

	struct dnsARecord : public dnsRecord
	{
		network_u32_t data;

		dnsARecord(std::string name, uint32_t ttl, network_u32_t data) noexcept
			: dnsRecord(DNS_A, std::move(name), ttl), data(data)
		{
		}

		[[nodiscard]] std::string toString() const final
		{
			return IpAddr(data).toString4();
		}
		
		[[nodiscard]] std::string toRdata() const final
		{
			return std::string(reinterpret_cast<const char*>(&data), 4);
		}
	};

	struct dnsAaaaRecord : public dnsRecord
	{
		IpAddr data;

		dnsAaaaRecord(std::string name, uint32_t ttl, const uint8_t* data)
			: dnsRecord(DNS_AAAA, std::move(name), ttl), data(data)
		{
		}

		dnsAaaaRecord(std::string name, uint32_t ttl, const IpAddr& data)
			: dnsRecord(DNS_AAAA, std::move(name), ttl), data(data)
		{
		}

		[[nodiscard]] std::string toString() const final
		{
			return data.toString6();
		}

		[[nodiscard]] std::string toRdata() const final
		{
			return std::string(reinterpret_cast<const char*>(&data.data), 16);
		}
	};

	struct dnsRecordName : public dnsRecord
	{
		std::string data;

		dnsRecordName(dnsType type, std::string&& name, uint32_t ttl, std::string&& data)
			: dnsRecord(type, std::move(name), ttl), data(std::move(data))
		{
		}

		[[nodiscard]] std::string toString() const final
		{
			return data;
		}

		[[nodiscard]] std::string toRdata() const final;
	};

	struct dnsCnameRecord : public dnsRecordName
	{
		dnsCnameRecord(std::string name, uint32_t ttl, std::string data)
			: dnsRecordName(DNS_CNAME, std::move(name), ttl, std::move(data))
		{
		}
	};

	struct dnsPtrRecord : public dnsRecordName
	{
		dnsPtrRecord(std::string name, uint32_t ttl, std::string data)
			: dnsRecordName(DNS_PTR, std::move(name), ttl, std::move(data))
		{
		}
	};

	struct dnsTxtRecord : public dnsRecord
	{
		std::string data;

		dnsTxtRecord(std::string name, uint32_t ttl, std::string data)
			: dnsRecord(DNS_TXT, std::move(name), ttl), data(std::move(data))
		{
		}

		[[nodiscard]] std::string toString() const final
		{
			return data;
		}

		[[nodiscard]] std::string toRdata() const final
		{
			std::string rdata = data;
			rdata.insert(0, 1, (char)(uint8_t)rdata.size());
			return rdata;
		}
	};

	struct dnsMxRecord : public dnsRecord
	{
		uint16_t priority;
		std::string target;

		dnsMxRecord(std::string name, uint32_t ttl, uint16_t priority, std::string target)
			: dnsRecord(DNS_MX, std::move(name), ttl), priority(priority), target(std::move(target))
		{
		}

		[[nodiscard]] std::string toString() const final;

		[[nodiscard]] std::string toRdata() const final;
	};

	struct dnsSrvRecord : public dnsRecord
	{
		uint16_t priority;
		uint16_t weight;
		std::string target;
		uint16_t port;

		dnsSrvRecord(std::string name, uint32_t ttl, uint16_t priority, uint16_t weight, std::string target, uint16_t port)
			: dnsRecord(DNS_SRV, std::move(name), ttl), priority(priority), weight(weight), target(std::move(target)), port(port)
		{
		}

		[[nodiscard]] std::string toString() const final;

		[[nodiscard]] std::string toRdata() const final;
	};

	struct dnsNsRecord : public dnsRecordName
	{
		dnsNsRecord(std::string name, uint32_t ttl, std::string data)
			: dnsRecordName(DNS_NS, std::move(name), ttl, std::move(data))
		{
		}
	};
}
