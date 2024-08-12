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
		std::vector<uint8_t> cipher_key;
		std::string mac_key;
		std::vector<uint8_t> implicit_iv;

		[[nodiscard]] bool isActive() const noexcept
		{
			return !cipher_key.empty();
		}

		[[nodiscard]] bool isAead() const noexcept
		{
			return !implicit_iv.empty();
		}

		[[nodiscard]] size_t getMacLength() const noexcept;
		[[nodiscard]] std::string calculateMacBytes(TlsContentType_t content_type, size_t content_length) SOUP_EXCAL;
		[[nodiscard]] std::string calculateMac(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL { return calculateMac(content_type, content.data(), content.size()); }
		[[nodiscard]] std::string calculateMac(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL;

		[[nodiscard]] Buffer encrypt(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL;

		void reset() noexcept;
	};
}
