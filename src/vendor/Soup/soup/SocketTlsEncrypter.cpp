#include "SocketTlsEncrypter.hpp"

#include "aes.hpp"
#include "rand.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "TlsMac.hpp"

NAMESPACE_SOUP
{
	size_t SocketTlsEncrypter::getMacLength() const noexcept
	{
		return mac_key.size();
	}

	std::string SocketTlsEncrypter::calculateMacBytes(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL
	{
		TlsMac mac{};
		mac.seq_num = seq_num++;
		mac.record.content_type = content_type;
		mac.record.length = static_cast<uint16_t>(content.size());
		return mac.toBinaryString();
	}

	std::string SocketTlsEncrypter::calculateMac(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL
	{
		auto msg = calculateMacBytes(content_type, content);
		msg.append(content);

		if (mac_key.size() == 20)
		{
			return sha1::hmac(msg, mac_key);
		}
		//else if (mac_key.size() == 32)
		{
			return sha256::hmac(msg, mac_key);
		}
	}

	std::vector<uint8_t> SocketTlsEncrypter::encrypt(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL
	{
		constexpr auto cipher_bytes = 16;

		if (!isAead()) // AES-CBC
		{
			constexpr auto record_iv_length = 16;

			auto mac = calculateMac(content_type, content);
			auto cont_with_mac_size = (content.size() + mac.size());
			auto aligned_in_len = ((((cont_with_mac_size + 1) / cipher_bytes) + 1) * cipher_bytes);
			auto pad_len = static_cast<char>(aligned_in_len - cont_with_mac_size);

			std::vector<uint8_t> data{};
			data.reserve(content.size() + mac.size() + pad_len);
			data.insert(data.end(), content.begin(), content.end());
			data.insert(data.end(), mac.begin(), mac.end());
			data.insert(data.end(), (size_t)pad_len, (pad_len - 1));

			auto iv = rand.vec_u8(record_iv_length);
			aes::cbcEncrypt(
				data.data(), data.size(),
				cipher_key.data(), cipher_key.size(),
				iv.data()
			);

			iv.insert(iv.end(), data.begin(), data.end());
			return iv;
		}
		else // AES-GCM
		{
			constexpr auto record_iv_length = 8;

			auto nonce_explicit = rand.vec_u8(record_iv_length);
			auto iv = implicit_iv;
			iv.insert(iv.end(), nonce_explicit.begin(), nonce_explicit.end());

			auto ad = calculateMacBytes(content_type, content);

			std::vector<uint8_t> data(content.begin(), content.end());

			uint8_t tag[cipher_bytes];
			aes::gcmEncrypt(
				data.data(), data.size(),
				(const uint8_t*)ad.data(), ad.size(),
				cipher_key.data(), cipher_key.size(),
				iv.data(), iv.size(),
				tag
			);

			data.insert(data.end(), tag, tag + cipher_bytes);
			data.insert(data.begin(), nonce_explicit.begin(), nonce_explicit.end());
			return data;
		}
	}

	void SocketTlsEncrypter::reset() noexcept
	{
		seq_num = 0;
		cipher_key.clear();
		mac_key.clear();
	}
}
