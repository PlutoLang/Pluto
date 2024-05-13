#include "IpAddr.hpp"

#include "Endian.hpp"
#include "IpGroups.hpp"
#include "netConfig.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	bool IpAddr::fromString(const char* str) noexcept
	{
		if (strstr(str, ":") != nullptr)
		{
			return inet_pton(AF_INET6, str, &data) == 1;
		}
		else
		{
			maskToV4();
			return inet_pton(AF_INET, str, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&data) + 12)) == 1;
		}
	}

	bool IpAddr::fromString(const std::string& str) noexcept
	{
		if (str.find(':') != std::string::npos)
		{
			return inet_pton(AF_INET6, str.data(), &data) == 1;
		}
		else
		{
			maskToV4();
			return inet_pton(AF_INET, str.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&data) + 12)) == 1;
		}
	}

	bool IpAddr::isLoopback() const noexcept
	{
		return isV4()
			? IpGroups::v4_loopback.contains(*this)
			: IpGroups::v6_loopback.contains(*this)
			;
	}

	bool IpAddr::isPrivate() const noexcept
	{
		return isV4() && IpGroups::v4_private_network.contains(*this);
	}

	bool IpAddr::isLocalnet() const noexcept
	{
		return isLoopback() || isPrivate();
	}

	native_u32_t IpAddr::getV4NativeEndian() const noexcept
	{
		return Endianness::toNative(getV4());
	}

	std::string IpAddr::getArpaName() const
	{
		if (isV4())
		{
			auto v4 = getV4();
			std::string str = std::to_string((v4 >> 24) & 0xFF);
			str.push_back('.');
			str.append(std::to_string((v4 >> 16) & 0xFF));
			str.push_back('.');
			str.append(std::to_string((v4 >> 8) & 0xFF));
			str.push_back('.');
			str.append(std::to_string(v4 & 0xFF));
			str.append(".in-addr.arpa");
			return str;
		}

		std::string str;
		for (const auto& b : string::bin2hexLower(std::string(reinterpret_cast<const char*>(&data), 16)))
		{
			str.insert(0, 1, '.');
			str.insert(0, 1, b);
		}
		str.append("in6.arpa");
		return str;
	}

#if !SOUP_WASM
	std::string IpAddr::getReverseDns() const
	{
		return getReverseDns(netConfig::get().getDnsResolver());
	}

	std::string IpAddr::getReverseDns(dnsResolver& resolver) const
	{
		if (auto records = resolver.lookup(DNS_PTR, getArpaName()))
		{
			for (const auto& record : *records)
			{
				if (record->type == DNS_PTR)
				{
					return static_cast<dnsPtrRecord*>(record.get())->data;
				}
			}
		}
		return {};
	}
#endif
}
