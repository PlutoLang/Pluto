#include "SocketTlsHandshaker.hpp"

#if !SOUP_WASM

#include "ObfusString.hpp"
#include "sha256.hpp"
#include "TlsHandshake.hpp"

namespace soup
{
	SocketTlsHandshaker::SocketTlsHandshaker(void(*callback)(Socket&, Capture&&) SOUP_EXCAL, Capture&& callback_capture) noexcept
		: callback(callback), callback_capture(std::move(callback_capture))
	{
		layer_bytes.reserve(2800); // When we receive "finished" from Cloudflare, this is 2689~2696 bytes.
	}

	std::string SocketTlsHandshaker::pack(TlsHandshakeType_t handshake_type, const std::string& content) SOUP_EXCAL
	{
		TlsHandshake hs{};
		hs.handshake_type = handshake_type;
		hs.length = static_cast<decltype(hs.length)>(content.size());
		auto data = hs.toBinaryString();
		data.append(content);

		layer_bytes.append(data);
		return data;
	}

	std::string SocketTlsHandshaker::getMasterSecret() SOUP_EXCAL
	{
		if (!pre_master_secret.empty())
		{
			if (extended_master_secret)
			{
				master_secret = sha256::tls_prf(
					ObfusString("extended master secret"),
					48,
					std::move(pre_master_secret),
					sha256::hash(layer_bytes)
				);
			}
			else
			{
				master_secret = sha256::tls_prf(
					ObfusString("master secret"),
					48,
					std::move(pre_master_secret),
					std::string(client_random).append(server_random)
				);
			}
			pre_master_secret.clear();
			pre_master_secret.shrink_to_fit();
		}
		return master_secret;
	}

	void SocketTlsHandshaker::getKeys(std::string& client_write_mac, std::string& server_write_mac, std::vector<uint8_t>& client_write_key, std::vector<uint8_t>& server_write_key, std::vector<uint8_t>& client_write_iv, std::vector<uint8_t>& server_write_iv) SOUP_EXCAL
	{
		size_t mac_key_length = 20; // SHA1 = 20, SHA256 = 32
		switch (cipher_suite)
		{
		case TLS_RSA_WITH_AES_128_CBC_SHA256:
		case TLS_RSA_WITH_AES_256_CBC_SHA256:
		case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
		case TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:
			mac_key_length = 32;
			break;

		case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
		//case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
		case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:
		//case TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384:
			mac_key_length = 0;
			break;
		}

		size_t enc_key_length = 16; // AES128 = 16, AES256 = 32
		switch (cipher_suite)
		{
		case TLS_RSA_WITH_AES_256_CBC_SHA:
		case TLS_RSA_WITH_AES_256_CBC_SHA256:
		case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
		case TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:
			enc_key_length = 32;
			break;
		}

		size_t fixed_iv_length = 0;
		switch (cipher_suite)
		{
		case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
		//case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
		case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:
		//case TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384:
			fixed_iv_length = 4;
			break;
		}

		// tls_prf should use sha384 for TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 & TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384 as per RFC5289.
		auto key_block = sha256::tls_prf(
			ObfusString("key expansion"),
			(mac_key_length * 2) + (enc_key_length * 2) + (fixed_iv_length * 2),
			getMasterSecret(),
			std::string(server_random).append(client_random)
		);

		client_write_mac = key_block.substr(0, mac_key_length);
		server_write_mac = key_block.substr(mac_key_length, mac_key_length);

		auto client_write_key_str = key_block.substr(mac_key_length * 2, enc_key_length);
		auto server_write_key_str = key_block.substr((mac_key_length * 2) + enc_key_length, enc_key_length);
		client_write_key = std::vector<uint8_t>(client_write_key_str.begin(), client_write_key_str.end());
		server_write_key = std::vector<uint8_t>(server_write_key_str.begin(), server_write_key_str.end());

		auto client_write_iv_str = key_block.substr((mac_key_length * 2) + (enc_key_length * 2), fixed_iv_length);
		auto server_write_iv_str = key_block.substr((mac_key_length * 2) + (enc_key_length * 2) + fixed_iv_length, fixed_iv_length);
		client_write_iv = std::vector<uint8_t>(client_write_iv_str.begin(), client_write_iv_str.end());
		server_write_iv = std::vector<uint8_t>(server_write_iv_str.begin(), server_write_iv_str.end());
	}

	std::string SocketTlsHandshaker::getClientFinishVerifyData() SOUP_EXCAL
	{
		return getFinishVerifyData(ObfusString("client finished"));
	}

	std::string SocketTlsHandshaker::getServerFinishVerifyData() SOUP_EXCAL
	{
		return getFinishVerifyData(ObfusString("server finished"));
	}

	std::string SocketTlsHandshaker::getFinishVerifyData(const std::string& label) SOUP_EXCAL
	{
		return sha256::tls_prf(label, 12, getMasterSecret(), sha256::hash(layer_bytes));
	}
}
#endif
