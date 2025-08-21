#pragma once

#include "base.hpp"
#include "fwd.hpp"

#include <cstdint>
#include <string>

NAMESPACE_SOUP
{
#if SOUP_BITS == 64
	using halfintmax_t = int32_t;
	using halfsize_t = uint32_t;
#elif SOUP_BITS == 32
	using halfintmax_t = int16_t;
	using halfsize_t = uint16_t;
#endif

	// net
	using certchain_validator_t = bool(*)(const X509Certchain&, const std::string&, StructMap&) SOUP_EXCAL;

	// net.tls
	using TlsAlertDescription_t = uint8_t;
	using TlsCipherSuite_t = uint16_t;
	using TlsContentType_t = uint8_t;
	using TlsHandshakeType_t = uint8_t;
	using tls_server_on_client_hello_t = void(*)(Socket&, TlsClientHello&&) SOUP_EXCAL;
	using tls_server_alpn_select_protocol_t = std::string(*)(Socket&, const TlsExtAlpn&, TlsCipherSuite_t) SOUP_EXCAL;

	// os
	using pid_t = unsigned int;
}
