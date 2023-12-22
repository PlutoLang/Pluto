#pragma once

#include <string>
#include <vector>

#include "base.hpp"
#include "type.hpp"

namespace soup
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
		[[nodiscard]] std::string calculateMacBytes(TlsContentType_t content_type, const std::string& content);
		[[nodiscard]] std::string calculateMac(TlsContentType_t content_type, const std::string& content);

		[[nodiscard]] std::vector<uint8_t> encrypt(TlsContentType_t content_type, const std::string& content);

		void reset() noexcept;
	};
}
