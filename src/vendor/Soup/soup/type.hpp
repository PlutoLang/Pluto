#pragma once

#include "fwd.hpp"

#include <cstdint>
#include <string>

namespace soup
{
	// net
	using certchain_validator_t = bool(*)(const X509Certchain&, const std::string&, StructMap&);

	// net.tls
	using TlsAlertDescription_t = uint8_t;
	using TlsCipherSuite_t = uint16_t;
	using TlsContentType_t = uint8_t;
	using TlsHandshakeType_t = uint8_t;
	using tls_server_cert_selector_t = void(*)(TlsServerRsaData& out, const std::string& server_name);
	using tls_server_on_client_hello_t = void(*)(Socket&, TlsClientHello&&);
}
