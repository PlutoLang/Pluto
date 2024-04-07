#pragma once

#include "fwd.hpp"

#include <string>

NAMESPACE_SOUP
{
	struct ServerServiceUdp
	{
		using callback_t = void(*)(Socket&, SocketAddr&&, std::string&&, ServerServiceUdp&);

		callback_t callback;

		ServerServiceUdp(callback_t callback)
			: callback(callback)
		{
		}
	};
}
