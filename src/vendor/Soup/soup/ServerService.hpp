#pragma once

#include "fwd.hpp"

NAMESPACE_SOUP
{
	struct ServerService
	{
		using callback_t = void(*)(Socket& client, ServerService&, Server&) SOUP_EXCAL;

		// on_connection_established is called when the TCP connection is established
		callback_t on_connection_established = nullptr;

		// on_tunnel_established is called when:
		// - (non-crypto) the TCP connection is established
		// - (crypto) the TLS handshake has completed
		const callback_t on_tunnel_established;

		ServerService(callback_t on_tunnel_established)
			: on_tunnel_established(on_tunnel_established)
		{
		}
	};
}
