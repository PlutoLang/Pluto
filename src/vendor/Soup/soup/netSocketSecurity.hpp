#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	enum netSocketSecurity : uint8_t
	{
		SOCKET_INSECURE,
		SOCKET_TLS,
		SOCKET_TLS_ECDHE,
	};
}
