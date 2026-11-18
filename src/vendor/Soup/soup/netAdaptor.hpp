#pragma once

#include "base.hpp"

#if SOUP_WINDOWS || SOUP_LINUX || SOUP_MACOS

#include <string>
#include <vector>

#include "Endian.hpp"
#include "MacAddr.hpp"

NAMESPACE_SOUP
{
	struct netAdaptor
	{
		std::string name;
		MacAddr mac_addr;
		native_u32_t ip_addr;
		native_u32_t netmask;

		[[nodiscard]] static std::vector<netAdaptor> getAll();

		[[nodiscard]] constexpr bool hasNetwork() const noexcept { return ip_addr != 0; }
		[[nodiscard]] bool isVirtual() const noexcept;
	};
}

#endif
