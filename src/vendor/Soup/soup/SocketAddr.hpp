#pragma once

#include "IpAddr.hpp"

namespace soup
{
	struct SocketAddr
	{
		IpAddr ip;
		network_u16_t port;

		SocketAddr() noexcept = default;

		explicit SocketAddr(const IpAddr& ip, network_u16_t port)
			: ip(ip), port(port)
		{
		}

		explicit SocketAddr(const IpAddr& ip, native_u16_t port)
			: ip(ip), port(Endianness::toNetwork(port))
		{
		}

		[[nodiscard]] native_u16_t getPort() const noexcept
		{
			return Endianness::toNative(port);
		}

		[[nodiscard]] std::string toString() const noexcept
		{
			std::string str;
			if (ip.isV4())
			{
				str.append(ip.toString4());
			}
			else
			{
				str.push_back('[');
				str.append(ip.toString6());
				str.push_back(']');
			}
			str.push_back(':');
			str.append(std::to_string(getPort()));
			return str;
		}

		[[nodiscard]] bool operator==(const SocketAddr& b) const noexcept
		{
			return ip == b.ip && port == b.port;
		}

		[[nodiscard]] bool operator!=(const SocketAddr& b) const noexcept
		{
			return !operator==(b);
		}
	};
}
