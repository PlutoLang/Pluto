#pragma once

#include <string>

namespace soup
{
	template <typename T>
	struct CryptoHashAlgo
	{
		[[nodiscard]] static std::string hashWithId(const std::string& msg) SOUP_EXCAL
		{
			auto hash_bin = T::hash(msg);
			prependId(hash_bin);
			return hash_bin;
		}

		static bool prependId(std::string& hash_bin) SOUP_EXCAL
		{
			if (hash_bin.length() != T::DIGEST_BYTES)
			{
				if (hash_bin.length() > T::DIGEST_BYTES)
				{
					return false;
				}
				hash_bin.insert(0, T::DIGEST_BYTES - hash_bin.length(), '\0');
			}
			hash_bin.insert(0, (const char*)T::OID, sizeof(T::OID)); // As per https://datatracker.ietf.org/doc/html/rfc3447#page-43
			return true;
		}

		[[nodiscard]] static std::string hmac(const std::string& msg, std::string key) SOUP_EXCAL
		{
			if (key.length() > T::BLOCK_BYTES)
			{
				key = T::hash(key);
			}

			std::string inner = key;
			std::string outer = key;

			for (size_t i = 0; i != key.length(); ++i)
			{
				inner[i] ^= 0x36;
				outer[i] ^= 0x5c;
			}

			if (auto diff = T::BLOCK_BYTES - key.length(); diff != 0)
			{
				inner.append(diff, '\x36');
				outer.append(diff, '\x5c');
			}

			inner.append(msg);
			outer.append(T::hash(std::move(inner)));
			return T::hash(std::move(outer));
		}

		// used as (secret, label, seed) in the RFC
		[[nodiscard]] static std::string tls_prf(std::string label, const size_t bytes, const std::string& secret, const std::string& seed) SOUP_EXCAL
		{
			std::string res{};

			label.append(seed);

			std::string A = label;
			do
			{
				A = hmac(A, secret);

				std::string round_key = A;
				round_key.append(label);

				res.append(hmac(round_key, secret));
			} while (res.size() < bytes);

			if (res.size() != bytes)
			{
				res.erase(bytes);
			}
			return res;
		}
	};
}
