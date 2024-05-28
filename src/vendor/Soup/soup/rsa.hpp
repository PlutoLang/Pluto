#pragma once

#include "fwd.hpp"

#include "Bigint.hpp"
#include "JsonObject.hpp"

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
