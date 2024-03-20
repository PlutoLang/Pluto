#pragma once

#include "fwd.hpp"

#include <string>

namespace soup
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
