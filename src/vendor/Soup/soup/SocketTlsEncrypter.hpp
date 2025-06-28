#pragma once

#include <string>
#include <vector>

#include "base.hpp"
#include "type.hpp"

#include "Buffer.hpp"

NAMESPACE_SOUP
{
	struct SocketTlsEncrypter
	{
		uint64_t seq_num = 0;
		uint8_t cipher_key[32];
		uint8_t mac_key[32];
		uint8_t implicit_iv[4];
		uint8_t cipher_key_len = 0;
		uint8_t mac_key_len = 0;
		uint8_t implicit_iv_len = 0;

		[[nodiscard]] bool isActive() const noexcept { return cipher_key_len != 0; }
		[[nodiscard]] bool isAead() const noexcept { return implicit_iv_len != 0; }
		[[nodiscard]] size_t getMacLength() const noexcept { return mac_key_len; }
		[[nodiscard]] std::string calculateMacBytes(TlsContentType_t content_type, size_t content_length) SOUP_EXCAL;
		[[nodiscard]] std::string calculateMac(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL { return calculateMac(content_type, content.data(), content.size()); }
		[[nodiscard]] std::string calculateMac(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL;

		[[nodiscard]] Buffer<> encrypt(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL;

		void reset() noexcept;
	};
}
