#pragma once

#include <string>

NAMESPACE_SOUP
{
	struct netReuseTag
	{
		std::string host;
		uint16_t port;
		bool tls;
		bool is_busy = true;

		void init(const std::string& host, uint16_t port, bool tls)
		{
			this->host = host;
			this->port = port;
			this->tls = tls;
		}
	};
}
