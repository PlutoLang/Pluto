#pragma once

#include "IpAddr.hpp"

NAMESPACE_SOUP
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

		bool fromString(const std::string& str) SOUP_EXCAL;

		[[nodiscard]] native_u16_t getPort() const noexcept
		{
			return Endianness::toNative(port);
		}

		[[nodiscard]] std::string toString() const noexcept
		{
			std::string str = ip.toStringForAddr();
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
