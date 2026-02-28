#pragma once

#include <string>

#include "netSocketSecurity.hpp"

NAMESPACE_SOUP
{
	struct netReuseTag
	{
		std::string host;
		uint16_t port;
		netSocketSecurity security;
		bool is_busy = true;

		void init(const std::string& host, uint16_t port, netSocketSecurity security)
		{
			this->host = host;
			this->port = port;
			this->security = security;
		}
	};
}
