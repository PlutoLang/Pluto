#pragma once

#include "fwd.hpp"

#include "Bigint.hpp"
#include "JsonObject.hpp"
#include "rand.hpp"

NAMESPACE_SOUP
{
	struct RsaMod
	{
		Bigint n{};

		RsaMod() noexcept = default;

		RsaMod(Bigint n) noexcept
			: n(std::move(n))
		{
		}

		[[nodiscard]] size_t getMaxUnpaddedMessageBytes() const noexcept;
		[[nodiscard]] size_t getMaxPkcs1MessageBytes() const noexcept;
		[[nodiscard]] size_t getMaxOaepMessageBytes(size_t digest_bytes) const noexcept;

		bool padPublic(std::string& str) const SOUP_EXCAL; // non-deterministic
		bool padPrivate(std::string& str) const SOUP_EXCAL; // deterministic

		template <typename CryptoHashAlgo>
		bool padHash(std::string& bin) const SOUP_EXCAL // deterministic
		{
			return CryptoHashAlgo::prependId(bin)
				&& padPrivate(bin)
				;
		}

		static bool unpad(std::string& str) noexcept;

		[[nodiscard]] UniquePtr<JsonObject> publicToJwk(const Bigint& e) const SOUP_EXCAL;
		[[nodiscard]] std::string publicGetJwkThumbprint(const Bigint& e) const SOUP_EXCAL;

		template <typename Hash>
		std::string padOaep(const std::string& msg, const char* label_data = "", size_t label_size = 0) const SOUP_EXCAL // non-deterministic
		{
			const auto k = getMaxUnpaddedMessageBytes();
			constexpr auto hLen = Hash::DIGEST_BYTES;
			const auto mLen = msg.size();

			SOUP_IF_UNLIKELY (k < (2 * hLen + 2) || mLen > (k - 2 * hLen - 2))
			{
				return {};
			}

			// Step 1
			uint8_t lHash[hLen];
			{
				typename Hash::State st;
				st.append(label_data, label_size);
				st.finalise();
				st.getDigest(lHash);
			}

			std::string db;
			db.reserve(k - hLen - 1);
			db.append((const char*)lHash, sizeof(lHash));
			db.append(k - mLen - 2 * hLen - 2, '\0'); // Step 2
			db.push_back('\x01');
			db.append(msg); // Step 3
			SOUP_DEBUG_ASSERT(db.size() == k - hLen - 1);

			// Step 4
			uint8_t seed[hLen];
			soup::rand.fill(seed);

			// Step 5
			const auto dbMask = Hash::mgf1(seed, sizeof(seed), db.size());

			// Step 6
			for (size_t i = 0; i != db.size(); ++i)
			{
				db[i] ^= dbMask[i];
			}

			// Step 7
			const auto seedMask = Hash::mgf1(db.data(), db.size(), sizeof(seed));

			// Step 8
			for (size_t i = 0; i != sizeof(seed); ++i)
			{
				seed[i] ^= seedMask[i];
			}

			// Step 9
			SOUP_DEBUG_ASSERT(1 + sizeof(seed) + db.size() == k);
			std::string res;
			res.reserve(k);
			res.push_back('\0');
			res.append((const char*)seed, sizeof(seed));
			res.append(db);
			return res;
		}

		template <typename Hash>
		static bool unpadOaep(std::string& str, const char* label_data = "", size_t label_size = 0) SOUP_EXCAL // deterministic
		{
			constexpr auto hLen = Hash::DIGEST_BYTES;

			SOUP_IF_UNLIKELY (str.size() < (2 * hLen + 2) || str[0] != '\0')
			{
				str.clear();
				return false;
			}

			// Reverse step 9
			uint8_t seed[hLen];
			memcpy(seed, str.data() + 1, hLen);
			auto db = str.substr(1 + hLen);

			// Reverse step 8 & 7
			const auto seedMask = Hash::mgf1(db.data(), db.size(), sizeof(seed));
			for (size_t i = 0; i != sizeof(seed); ++i)
			{
				seed[i] ^= seedMask[i];
			}

			// Reverse step 6 & 5
			const auto dbMask = Hash::mgf1(seed, sizeof(seed), db.size());
			for (size_t i = 0; i != db.size(); ++i)
			{
				db[i] ^= dbMask[i];
			}

			// Verify label
			uint8_t lHash[hLen];
			{
				typename Hash::State st;
				st.append(label_data, label_size);
				st.finalise();
				st.getDigest(lHash);
			}
			SOUP_IF_UNLIKELY (memcmp(lHash, db.data(), sizeof(lHash)) != 0)
			{
				str.clear();
				return false;
			}

			// Final push
			size_t i = sizeof(lHash);
			for (; i != db.size(); ++i)
			{
				if (db[i] == 1)
				{
					++i;
					break;
				}
				SOUP_IF_UNLIKELY (db[i] != 0)
				{
					str.clear();
					return false;
				}
			}
			str = db.substr(i);
			return true;
		}
	};

	template <typename T>
	struct RsaKey : public RsaMod
	{
		using RsaMod::RsaMod;

		[[nodiscard]] Bigint encryptUnpadded(const std::string& msg) const SOUP_EXCAL // deterministic
		{
			return static_cast<const T*>(this)->modPow(Bigint::fromBinary(msg));
		}

		[[nodiscard]] std::string decryptUnpadded(const Bigint& enc) const SOUP_EXCAL
		{
			return static_cast<const T*>(this)->modPow(enc).toBinary();
		}

		template <typename Hash>
		[[nodiscard]] Bigint encryptOaep(const std::string& msg) const SOUP_EXCAL
		{
			return encryptUnpadded(padOaep<Hash>(msg));
		}

		template <typename Hash>
		[[nodiscard]] std::string decryptOaep(const Bigint& enc) const SOUP_EXCAL
		{
			const auto k = getMaxUnpaddedMessageBytes();
			auto msg = static_cast<const T*>(this)->modPow(enc).toBinary(k);
			unpadOaep<Hash>(msg);
			return msg;
		}

		// With 2048-bit private key, OpenSSL takes ~0.001 ms. Soup takes ~12.4 ms. What the fuck?
		[[nodiscard]] std::string decryptPkcs1(const Bigint& enc) const SOUP_EXCAL
		{
			auto msg = decryptUnpadded(enc);
			unpad(msg);
			return msg;
		}
	};

	template <typename T>
	struct RsaPublicKeyBase : public RsaKey<T>
	{
		Bigint e{};

		RsaPublicKeyBase() noexcept = default;

		RsaPublicKeyBase(Bigint n, Bigint e) noexcept
			: RsaKey<T>(std::move(n)), e(std::move(e))
		{
		}

		[[nodiscard]] Bigint encryptPkcs1(std::string msg) const // non-deterministic
		{
			RsaKey<T>::padPublic(msg);
			return RsaKey<T>::encryptUnpadded(msg);
		}

		template <typename CryptoHashAlgo>
		[[nodiscard]] bool verify(const std::string& msg, const Bigint& sig) const SOUP_EXCAL
		{
			auto hash_bin = CryptoHashAlgo::hash(msg);
			return RsaKey<T>::template padHash<CryptoHashAlgo>(hash_bin)
				&& RsaKey<T>::decryptUnpadded(sig) == hash_bin;
		}

		[[nodiscard]] UniquePtr<JsonObject> toJwk() const
		{
			return RsaKey<T>::publicToJwk(e);
		}

		[[nodiscard]] std::string getJwkThumbprint() const
		{
			return RsaKey<T>::publicGetJwkThumbprint(e);
		}
	};

	struct RsaPublicKey : public RsaPublicKeyBase<RsaPublicKey>
	{
		static Bigint E_PREF;

		RsaPublicKey() noexcept = default;
		RsaPublicKey(Bigint n) noexcept;
		RsaPublicKey(Bigint n, Bigint e) noexcept;

		[[nodiscard]] Bigint modPow(const Bigint& x) const SOUP_EXCAL;
	};

	struct RsaKeyMontgomeryData
	{
		size_t re{};
		Bigint r{};
		Bigint n_mod_mul_inv{};
		Bigint r_mod_mul_inv{};
		Bigint one_mont{};

		RsaKeyMontgomeryData() noexcept = default;
		RsaKeyMontgomeryData(const Bigint& n);

		[[nodiscard]] Bigint modPow(const Bigint& n, const Bigint& e, const Bigint& x) const SOUP_EXCAL;
	};

	/*
	* In the case of an 1024-bit rsa public key, using a long-lived instance takes ~134ms, but performs operations in ~2ms, compared to
	* ~12ms using a short-lived instance. From these numbers, we can estimate that a long-lived instance is the right choice for rsa public
	* keys that are (expected to be) used more than 13 times.
	*/
	struct RsaPublicKeyLonglived : public RsaPublicKeyBase<RsaPublicKeyLonglived>
	{
		RsaKeyMontgomeryData mont_data;

		RsaPublicKeyLonglived() = default;
		RsaPublicKeyLonglived(Bigint n);
		RsaPublicKeyLonglived(Bigint n, Bigint e);

		[[nodiscard]] Bigint modPow(const Bigint& x) const SOUP_EXCAL;
	};

	struct RsaPrivateKey : public RsaKey<RsaPrivateKey>
	{
		Bigint p;
		Bigint q;
		Bigint dp;
		Bigint dq;
		Bigint qinv;

		RsaKeyMontgomeryData p_mont_data;
		RsaKeyMontgomeryData q_mont_data;

		RsaPrivateKey() = default;
		RsaPrivateKey(Bigint n, Bigint p, Bigint q, Bigint dp, Bigint dq, Bigint qinv);

		[[nodiscard]] static RsaPrivateKey fromPrimes(Bigint p, Bigint q);

		[[nodiscard]] static RsaPrivateKey fromPem(const std::string& data);
		[[nodiscard]] static RsaPrivateKey fromDer(const std::string& bin);
		[[nodiscard]] static RsaPrivateKey fromAsn1(const Asn1Sequence& seq);
		[[nodiscard]] static RsaPrivateKey fromJwk(const JsonObject& jwk);

		template <typename CryptoHashAlgo>
		[[nodiscard]] Bigint sign(const std::string& msg) const SOUP_EXCAL // deterministic
		{
			return encryptPkcs1(CryptoHashAlgo::hashWithId(msg));
		}

		[[nodiscard]] Bigint encryptPkcs1(std::string msg) const SOUP_EXCAL; // deterministic

		[[nodiscard]] RsaPublicKey derivePublic() const; // assumes that e = e_pref, which is true unless your keypair is 21-bit or less.

		[[nodiscard]] Asn1Sequence toAsn1() const; // as per PKCS #1. assumes that e = e_pref, which is true unless your keypair is 21-bit or less.
		[[nodiscard]] std::string toPem() const; // assumes that e = e_pref, which is true unless your keypair is 21-bit or less.

		[[nodiscard]] Bigint modPow(const Bigint& x) const SOUP_EXCAL;
		[[nodiscard]] Bigint getE() const; // returns public exponent. assumes that e = e_pref, which is true unless your keypair is 21-bit or less.
		[[nodiscard]] Bigint getD() const; // returns private exponent. assumes that e = e_pref, which is true unless your keypair is 21-bit or less.

		[[nodiscard]] Asn1Sequence createCsr(const std::vector<std::string>& common_names) const; // returns a Certificate Signing Request as per PKCS #10.

		template <typename T>
		bool io(T& s)
		{
			if (p.io(s)
				&& q.io(s)
				)
			{
				if constexpr (T::isRead())
				{
					*this = fromPrimes(p, q);
				}
				return true;
			}
			return false;
		}
	};

	struct RsaKeypair : public RsaMod
	{
		Bigint p;
		Bigint q;
		Bigint e;
		Bigint dp;
		Bigint dq;
		Bigint qinv;

		RsaKeypair() = default;
		RsaKeypair(Bigint _p, Bigint _q);

		[[nodiscard]] static RsaKeypair generate(unsigned int bits, bool lax_length_requirement = false);
		[[nodiscard]] static RsaKeypair generate(StatelessRngInterface& rng, unsigned int bits, bool lax_length_requirement = false);
		[[nodiscard]] static RsaKeypair generate(RngInterface& rng, RngInterface& aux_rng, unsigned int bits, bool lax_length_requirement = false);

		[[nodiscard]] RsaPublicKey getPublic() const SOUP_EXCAL;
		[[nodiscard]] RsaPrivateKey getPrivate() const SOUP_EXCAL;
	};
}
