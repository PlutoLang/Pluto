#pragma once

#include "base.hpp"
#if !SOUP_WASM
#include "fwd.hpp"
#include "type.hpp"

#include "Capture.hpp"
#include "CertStore.hpp"
#include "Promise.hpp"
#include "SharedPtr.hpp"
#include "SocketTlsEncrypter.hpp"
#include "TlsCipherSuite.hpp"
#include "X509Certchain.hpp"
#include "X509Certificate.hpp"

NAMESPACE_SOUP
{
	class SocketTlsHandshaker
	{
	public:
		void(*callback)(Socket&, Capture&&);
		Capture callback_capture;

		TlsCipherSuite_t cipher_suite = TLS_RSA_WITH_AES_128_CBC_SHA;
		uint16_t ecdhe_curve = 0; // client
		Promise<> promise{};
		bool extended_master_secret = false;
		std::string layer_bytes{};
		std::string client_random{};
		std::string server_random{};
		std::string pre_master_secret{};
		std::string master_secret{};
		std::string expected_finished_verify_data{};

		// client
		X509Certchain certchain{};
		std::string server_name{};
		std::string ecdhe_public_key{};
		std::string initial_application_data{};
		SocketTlsEncrypter pending_recv_encrypter;

		// server
		SharedPtr<CertStore> certstore;
		void(*on_client_hello)(Socket&, TlsClientHello&&);
		const RsaPrivateKey* private_key{};

		explicit SocketTlsHandshaker(void(*callback)(Socket&, Capture&&), Capture&& callback_capture) noexcept;

		[[nodiscard]] std::string pack(TlsHandshakeType_t handshake_type, const std::string& content) SOUP_EXCAL;

		[[nodiscard]] std::string getMasterSecret() SOUP_EXCAL;
		void getKeys(std::string& client_write_mac, std::string& server_write_mac, std::vector<uint8_t>& client_write_key, std::vector<uint8_t>& server_write_key, std::vector<uint8_t>& client_write_iv, std::vector<uint8_t>& server_write_iv) SOUP_EXCAL;

		[[nodiscard]] std::string getClientFinishVerifyData() SOUP_EXCAL;
		[[nodiscard]] std::string getServerFinishVerifyData() SOUP_EXCAL;
	private:
		[[nodiscard]] std::string getFinishVerifyData(const std::string& label) SOUP_EXCAL;
		[[nodiscard]] std::string getPseudoRandomBytes(const std::string& label, const size_t bytes, const std::string& secret, const std::string& seed) const SOUP_EXCAL;
		[[nodiscard]] std::string getLayerBytesHash() const SOUP_EXCAL;
	};
}
#endif
