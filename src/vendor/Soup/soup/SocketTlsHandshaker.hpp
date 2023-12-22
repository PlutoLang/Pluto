#pragma once

#include "base.hpp"
#if !SOUP_WASM
#include "fwd.hpp"
#include "type.hpp"

#include "Capture.hpp"
#include "Promise.hpp"
#include "SocketTlsEncrypter.hpp"
#include "TlsServerRsaData.hpp"
#include "TlsCipherSuite.hpp"
#include "X509Certchain.hpp"
#include "X509Certificate.hpp"

namespace soup
{
	class SocketTlsHandshaker
	{
	public:
		void(*callback)(Socket&, Capture&&);
		Capture callback_capture;

		TlsCipherSuite_t cipher_suite = TLS_RSA_WITH_AES_128_CBC_SHA;
		std::string layer_bytes{};
		std::string client_random{};
		std::string server_random{};
		std::string pre_master_secret{};
		std::string master_secret{};
		std::string expected_finished_verify_data{};
		Promise<> promise{};

		// client
		X509Certchain certchain{};
		std::string server_name{};
		uint16_t ecdhe_curve = 0;
		std::string ecdhe_public_key{};
		SocketTlsEncrypter pending_recv_encrypter;

		// server
		tls_server_cert_selector_t cert_selector;
		void(*on_client_hello)(Socket&, TlsClientHello&&);
		TlsServerRsaData rsa_data{};

		explicit SocketTlsHandshaker(void(*callback)(Socket&, Capture&&), Capture&& callback_capture);

		[[nodiscard]] std::string pack(TlsHandshakeType_t handshake_type, const std::string& content);

		[[nodiscard]] std::string getMasterSecret();
		void getKeys(std::string& client_write_mac, std::string& server_write_mac, std::vector<uint8_t>& client_write_key, std::vector<uint8_t>& server_write_key, std::vector<uint8_t>& client_write_iv, std::vector<uint8_t>& server_write_iv);

		[[nodiscard]] std::string getClientFinishVerifyData();
		[[nodiscard]] std::string getServerFinishVerifyData();
	private:
		[[nodiscard]] std::string getFinishVerifyData(const std::string& label);
	};
}
#endif
