#include "SocketTlsEncrypter.hpp"

#include "aes.hpp"
#include "md5.hpp"
#include "rand.hpp"
#include "Rc4State.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "TlsMac.hpp"

NAMESPACE_SOUP
{
	std::string SocketTlsEncrypter::calculateMacBytes(TlsContentType_t content_type, size_t content_length) SOUP_EXCAL
	{
		TlsMac mac{};
		mac.seq_num = seq_num++;
		mac.record.content_type = content_type;
		mac.record.length = static_cast<uint16_t>(content_length);
		return mac.toBinaryString();
	}

	std::string SocketTlsEncrypter::calculateMac(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL
	{
		auto msg = calculateMacBytes(content_type, size);
		if (mac_key_len == 20)
		{
			sha1::HmacState st(mac_key, mac_key_len);
			st.append(msg.data(), msg.size());
			st.append(data, size);
			st.finalise();
			return st.getDigest();
		}
		else if (mac_key_len == 32)
		{
			sha256::HmacState st(mac_key, mac_key_len);
			st.append(msg.data(), msg.size());
			st.append(data, size);
			st.finalise();
			return st.getDigest();
		}
		//else if (mac_key_len == 16)
		{
			md5::HmacState st(mac_key, mac_key_len);
			st.append(msg.data(), msg.size());
			st.append(data, size);
			st.finalise();
			return st.getDigest();
		}
	}

	Buffer<> SocketTlsEncrypter::encrypt(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL
	{
		constexpr auto cipher_bytes = 16; // AES = 16, RC4 = 1

		if (!isAead()) // AES-CBC
		{
			auto mac = calculateMac(content_type, data, size);

			SOUP_IF_UNLIKELY (isRc4())
			{
				Buffer buf;
				buf.reserve(size + mac.size());
				buf.append(data, size);
				buf.append(mac.data(), mac.size());
				if (!state)
				{
					state = new Rc4State(cipher_key, cipher_key_len);
				}
				reinterpret_cast<Rc4State*>(state)->transform(buf.data(), buf.size());
				return buf;
			}

			constexpr auto record_iv_length = 16;

			auto cont_with_mac_size = (size + mac.size());
			auto aligned_in_len = ((((cont_with_mac_size + 1) / cipher_bytes) + 1) * cipher_bytes);
			auto pad_len = static_cast<char>(aligned_in_len - cont_with_mac_size);

			Buffer buf;
			buf.reserve(size + mac.size() + pad_len);
			buf.append(data, size);
			buf.append(mac.data(), mac.size());
			buf.insert_back((size_t)pad_len, (pad_len - 1));

			auto iv = rand.vec_u8(record_iv_length);
			aes::cbcEncrypt(
				buf.data(), buf.size(),
				cipher_key, cipher_key_len,
				iv.data()
			);

			buf.prepend(iv.data(), iv.size());
			return buf;
		}
		else // AES-GCM
		{
			constexpr auto implicit_iv_len = 4;
			constexpr auto explicit_iv_len = 8;

			uint8_t iv[implicit_iv_len + explicit_iv_len];
			memcpy(iv, implicit_iv, implicit_iv_len);
			for (auto i = implicit_iv_len; i != implicit_iv_len + explicit_iv_len; ++i)
			{
				iv[i] = rand.byte();
			}

			auto ad = calculateMacBytes(content_type, size);

			Buffer buf;
			buf.reserve(size + cipher_bytes + explicit_iv_len);
			buf.append(&iv[implicit_iv_len], explicit_iv_len);
			buf.append(data, size);

			uint8_t tag[cipher_bytes];
			aes::gcmEncrypt(
				buf.data() + explicit_iv_len, buf.size() - explicit_iv_len,
				(const uint8_t*)ad.data(), ad.size(),
				cipher_key, cipher_key_len,
				iv, sizeof(iv),
				tag
			);

			buf.append(tag, cipher_bytes);
			return buf;
		}
	}

	void SocketTlsEncrypter::reset() noexcept
	{
		seq_num = 0;
		cipher_key_len = 0;
		mac_key_len = 0;
	}

	SocketTlsEncrypter::~SocketTlsEncrypter() noexcept
	{
		//if (state != nullptr)
		{
			delete reinterpret_cast<Rc4State*>(state);
		}
	}
}
