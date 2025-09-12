#pragma once

#include "base.hpp"

#include <cstdint> // uint8_t
#include <cstring> // memset, memcpy
#include <string>

#include "Endian.hpp"

NAMESPACE_SOUP
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

		[[nodiscard]] static std::string hmac(const std::string& msg, const std::string& key) SOUP_EXCAL
		{
			HmacState st(key);
			st.append(msg.data(), msg.size());
			st.finalise();
			return st.getDigest();
		}

		[[nodiscard]] static std::string mgf1(const void* seed_data, size_t seed_size, size_t out_len)
		{
			std::string out;
			out.reserve(((out_len / T::DIGEST_BYTES) + 1) * T::DIGEST_BYTES);
			for (uint32_t counter = 0; out.size() < out_len; ++counter)
			{
				const auto counter_be = Endianness::toNetwork(counter);

				typename T::State st;
				st.append(seed_data, seed_size);
				st.append(&counter_be, 4);
				st.finalise();
				out.append(st.getDigest());
			}
			if (size_t x = out.size() - out_len)
			{
				out.erase(out_len, x);
			}
			return out;
		}

		// used as (secret, label, seed) in the RFC
		[[nodiscard]] static std::string tls_prf(const std::string& label, const size_t bytes, const std::string& secret, const std::string& seed) SOUP_EXCAL
		{
			std::string res{};

			uint8_t A[T::DIGEST_BYTES];

			// Round 1: Initialise A
			// Typically, every round does 'A = hmac(A, secret)', with the assumption that A is initialised to 'label + seed'.
			// Here, we do this in one fell swoop.
			{
				HmacState st(secret);
				st.append(label.data(), label.size());
				st.append(seed.data(), seed.size());
				st.finalise();
				st.getDigest(A);
			}

			// Round 1: Append bytes
			{
				HmacState st(secret);
				st.append(A, sizeof(A));
				st.append(label.data(), label.size());
				st.append(seed.data(), seed.size());
				st.finalise();
				res.append(st.getDigest());
			}

			// Round 2+
			while (res.size() < bytes)
			{
				// Update A
				{
					HmacState st(secret);
					st.append(A, sizeof(A));
					st.finalise();
					st.getDigest(A);
				}

				// Append bytes
				{
					HmacState st(secret);
					st.append(A, sizeof(A));
					st.append(label.data(), label.size());
					st.append(seed.data(), seed.size());
					st.finalise();
					res.append(st.getDigest());
				}
			}

			if (res.size() != bytes)
			{
				res.erase(bytes);
			}
			return res;
		}

		[[nodiscard]] static std::string hkdf_extract(const std::string& salt, const std::string& ikm) SOUP_EXCAL
		{
			std::string key = salt;
			if (key.empty())
			{
				key.resize(T::DIGEST_BYTES, '\0');
			}
			HmacState st(key);
			st.append(ikm.data(), ikm.size());
			st.finalise();
			return st.getDigest();
		}

		[[nodiscard]] static std::string hkdf_expand(const std::string& prk, const std::string& info, size_t bytes) SOUP_EXCAL
		{
			std::string okm{};
			okm.reserve(((bytes / T::DIGEST_BYTES) + 1) * T::DIGEST_BYTES);

			std::string t{};
			uint8_t counter = 1;
			while (okm.size() < bytes)
			{
				HmacState st(prk);
				if (!t.empty())
				{
					st.append(t.data(), t.size());
				}
				st.append(info.data(), info.size());
				st.append(&counter, 1);
				st.finalise();
				t = st.getDigest();
				okm.append(t);
				++counter;
			}

			if (okm.size() != bytes)
			{
				okm.erase(bytes);
			}
			return okm;
		}

		[[nodiscard]] static std::string hkdf(const std::string& salt, const std::string& ikm, const std::string& info, size_t bytes) SOUP_EXCAL
		{
			return hkdf_expand(hkdf_extract(salt, ikm), info, bytes);
		}

		struct HmacState
		{
			using Hash = T;

			typename T::State inner;
			typename T::State outer;

			HmacState(const std::string& key) noexcept
				: HmacState(key.data(), key.size())
			{
			}

			HmacState(const void* key_data, size_t key_size) noexcept
			{
				uint8_t header[T::BLOCK_BYTES];
				memset(header, 0, sizeof(header));

				if (key_size <= T::BLOCK_BYTES)
				{
					memcpy(header, key_data, key_size);
				}
				else
				{
					typename T::State st;
					st.append(key_data, key_size);
					st.finalise();
					st.getDigest(header); static_assert(T::DIGEST_BYTES <= T::BLOCK_BYTES);
				}

				for (size_t i = 0; i != sizeof(header); ++i)
				{
					inner.appendByte(header[i] ^ 0x36);
					outer.appendByte(header[i] ^ 0x5c);
				}
			}

			void append(const void* data, size_t size) noexcept
			{
				inner.append(data, size);
			}

			void finalise() noexcept
			{
				uint8_t buf[T::DIGEST_BYTES];
				inner.finalise();
				inner.getDigest(buf);
				outer.append(buf, sizeof(buf));
				outer.finalise();
			}

			void getDigest(uint8_t out[T::DIGEST_BYTES]) const noexcept
			{
				return outer.getDigest(out);
			}

			[[nodiscard]] std::string getDigest() const SOUP_EXCAL
			{
				return outer.getDigest();
			}
		};
	};
}
