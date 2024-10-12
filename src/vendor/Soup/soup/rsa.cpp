#include "rsa.hpp"

#include "Asn1Sequence.hpp"
#include "Asn1Type.hpp"
#include "base64.hpp"
#include "DefaultRngInterface.hpp"
#include "HardwareRng.hpp"
#include "JsonString.hpp"
#include "ObfusString.hpp"
#include "pem.hpp"
#include "Promise.hpp"
#include "rand.hpp"
#include "sha256.hpp"
#include "X509RelativeDistinguishedName.hpp"

NAMESPACE_SOUP
{
	using namespace literals;

	// Mod

	size_t RsaMod::getMaxUnpaddedMessageBytes() const noexcept
	{
		return n.getNumBytes();
	}

	size_t RsaMod::getMaxPkcs1MessageBytes() const noexcept
	{
		return getMaxUnpaddedMessageBytes() - 11;
	}

	bool RsaMod::padPublic(std::string& str) const SOUP_EXCAL
	{
		const auto len = str.length();
		const auto max_unpadded_len = getMaxUnpaddedMessageBytes();
		if (len + 11 > max_unpadded_len)
		{
			return false;
		}
		str.reserve(max_unpadded_len);
		str.insert(0, 1, '\0');
		for (size_t i = max_unpadded_len - len - 3; i != 0; --i)
		{
			str.insert(0, 1, rand.ch(1u));
		}
		str.insert(0, 1, '\2');
		//str.insert(0, 1, '\0');
		return true;
	}

	bool RsaMod::padPrivate(std::string& str) const SOUP_EXCAL
	{
		const auto len = str.length();
		const auto max_unpadded_len = getMaxUnpaddedMessageBytes();
		if (len + 11 > max_unpadded_len)
		{
			return false;
		}
		str.reserve(max_unpadded_len);
		str.insert(0, 1, '\0');
		str.insert(0, max_unpadded_len - len - 3, '\xff');
		str.insert(0, 1, '\1');
		//str.insert(0, 1, '\0');
		return true;
	}

	bool RsaMod::unpad(std::string& str) noexcept
	{
		size_t len = str.length();
		if (len > 11)
		{
			auto c = str.data();
			//if (*c++ == 0)
			{
				if (auto type = *c++; type == 1 || type == 2)
				{
					while (*c++ != '\0');
					str.erase(0, c - str.data());
					return true;
				}
			}
		}
		return false;
	}

	UniquePtr<JsonObject> RsaMod::publicToJwk(const Bigint& e) const SOUP_EXCAL
	{
		auto obj = soup::make_unique<JsonObject>();
		obj->add("kty", "RSA");
		obj->add("n", base64::urlEncode(n.toBinary()));
		obj->add("e", base64::urlEncode(e.toBinary()));
		return obj;
	}

	std::string RsaMod::publicGetJwkThumbprint(const Bigint& e) const SOUP_EXCAL
	{
		auto jwk = publicToJwk(e);
		std::sort(jwk->children.begin(), jwk->children.end(), [](const std::pair<UniquePtr<JsonNode>, UniquePtr<JsonNode>>& a, const std::pair<UniquePtr<JsonNode>, UniquePtr<JsonNode>>& b)
		{
			return *a.first < *b.first;
		});
		return sha256::hash(jwk->encode());
	}

	// KeyMontgomeryData

	RsaKeyMontgomeryData::RsaKeyMontgomeryData(const Bigint& n)
		: re(n.montgomeryREFromM()),
		r(Bigint::montgomeryRFromRE(re)),
		one_mont(r.modUnsignedNotpowerof2(n))
	{
		Bigint::modMulInv2Coprimes(n, r, n_mod_mul_inv, r_mod_mul_inv);
	}

	Bigint RsaKeyMontgomeryData::modPow(const Bigint& n, const Bigint& e, const Bigint& x) const SOUP_EXCAL
	{
		return x.modPowMontgomery(e, re, r, n, r_mod_mul_inv, n_mod_mul_inv, one_mont);
	}

	// PublicKey

#define E_PREF_VAL 65537_b // 0x10001

	Bigint RsaPublicKey::E_PREF = E_PREF_VAL;

	RsaPublicKey::RsaPublicKey(Bigint n) noexcept
		: RsaPublicKey(std::move(n), E_PREF_VAL)
	{
	}

	RsaPublicKey::RsaPublicKey(Bigint n, Bigint e) noexcept
		: RsaPublicKeyBase(std::move(n), std::move(e))
	{
	}

	Bigint RsaPublicKey::modPow(const Bigint& x) const SOUP_EXCAL
	{
		return x.modPowBasic(e, n);
	}

	// LonglivedPublicKey

	RsaPublicKeyLonglived::RsaPublicKeyLonglived(Bigint n)
		: RsaPublicKeyLonglived(std::move(n), E_PREF_VAL)
	{
	}

	RsaPublicKeyLonglived::RsaPublicKeyLonglived(Bigint n, Bigint e)
		: RsaPublicKeyBase(std::move(n), std::move(e)), mont_data(this->n)
	{
	}

	Bigint RsaPublicKeyLonglived::modPow(const Bigint& x) const SOUP_EXCAL
	{
		return mont_data.modPow(n, e, x);
	}

	// PrivateKey

	RsaPrivateKey::RsaPrivateKey(Bigint n, Bigint p, Bigint q, Bigint dp, Bigint dq, Bigint qinv)
		: RsaKey(std::move(n)), p(std::move(p)), q(std::move(q)), dp(std::move(dp)), dq(std::move(dq)), qinv(std::move(qinv)),
		p_mont_data(this->p),
		q_mont_data(this->q)
	{
	}

	RsaPrivateKey RsaPrivateKey::fromPrimes(Bigint p, Bigint q)
	{
		return RsaKeypair(std::move(p), std::move(q)).getPrivate();
	}

	RsaPrivateKey RsaPrivateKey::fromPem(const std::string& data)
	{
		return fromDer(pem::decode(data));
	}

	RsaPrivateKey RsaPrivateKey::fromDer(const std::string& bin)
	{
		return fromAsn1(Asn1Sequence::fromDer(bin));
	}

	RsaPrivateKey RsaPrivateKey::fromAsn1(const Asn1Sequence& seq)
	{
		if (seq.getChildType(1).type != ASN1_INTEGER)
		{
			// assuming that seq[1] is sequence containing OID 1.2.840.113549.1.1.1
			return fromAsn1(Asn1Sequence::fromDer(seq.getString(2)));
		}
		return {
			seq.getInt(1),
			seq.getInt(4),
			seq.getInt(5),
			seq.getInt(6),
			seq.getInt(7),
			seq.getInt(8),
		};
	}

	RsaPrivateKey RsaPrivateKey::fromJwk(const JsonObject& jwk)
	{
		// assuming that jwk["kty"] == "RSA"
		return {
			Bigint::fromBinary(base64::urlDecode(jwk.at("n").asStr())),
			Bigint::fromBinary(base64::urlDecode(jwk.at("p").asStr())),
			Bigint::fromBinary(base64::urlDecode(jwk.at("q").asStr())),
			Bigint::fromBinary(base64::urlDecode(jwk.at("dp").asStr())),
			Bigint::fromBinary(base64::urlDecode(jwk.at("dq").asStr())),
			Bigint::fromBinary(base64::urlDecode(jwk.at("qi").asStr())),
		};
	}

	Bigint RsaPrivateKey::encryptPkcs1(std::string msg) const SOUP_EXCAL
	{
		padPrivate(msg);
		return encryptUnpadded(msg);
	}

	RsaPublicKey RsaPrivateKey::derivePublic() const
	{
		return RsaPublicKey(n);
	}

	Asn1Sequence RsaPrivateKey::toAsn1() const
	{
		Asn1Sequence seq{};
		/* 0 */ seq.addInt({}); // version (0)
		/* 1 */ seq.addInt(n);
		/* 2 */ seq.addInt(getE());
		/* 3 */ seq.addInt(getD());
		/* 4 */ seq.addInt(p);
		/* 5 */ seq.addInt(q);
		/* 6 */ seq.addInt(dp);
		/* 7 */ seq.addInt(dq);
		/* 8 */ seq.addInt(qinv);
		return seq;
	}

	std::string RsaPrivateKey::toPem() const
	{
		return pem::encode(ObfusString("RSA PRIVATE KEY"), toAsn1().toDer());
	}

	Bigint RsaPrivateKey::modPow(const Bigint& x) const SOUP_EXCAL
	{
		auto mp = p_mont_data.modPow(p, dp, x);
		auto mq = q_mont_data.modPow(q, dq, x);
		auto h = (qinv * (mp - mq) % p);
		return ((mq + (h * q)) % n);
	}

	Bigint RsaPrivateKey::getE() const
	{
		return RsaPublicKey::E_PREF;
	}

	Bigint RsaPrivateKey::getD() const
	{
		return getE().modMulInv((p - 1_b).lcm(q - 1_b));
	}

	Asn1Sequence RsaPrivateKey::createCsr(const std::vector<std::string>& common_names) const
	{
		Asn1Sequence certReq;
		std::string tbs;
		{
			Asn1Sequence certReqInfo;
			certReqInfo.addInt({}); // version (0)
			{
				X509RelativeDistinguishedName subject;
				for (const auto& common_name : common_names)
				{
					subject.emplace_back(Oid::COMMON_NAME, common_name);
				}
				certReqInfo.addName(subject);
			}
			{
				Asn1Sequence subjectPublicKeyInfo;
				{
					Asn1Sequence algorithm;
					algorithm.addOid(Oid::RSA_ENCRYPTION);
					algorithm.addNull();
					subjectPublicKeyInfo.addSeq(algorithm);
				}
				{
					Asn1Sequence subjectPublicKey;
					subjectPublicKey.addInt(n);
					subjectPublicKey.addInt(getE());
					subjectPublicKeyInfo.addBitString(subjectPublicKey.toDer());
				}
				certReqInfo.addSeq(subjectPublicKeyInfo);
			}
			certReqInfo.emplace_back(Asn1Element{
				Asn1Identifier{ 2, true, 0 },
				{}
			});
			tbs = certReqInfo.toDer();
			certReq.addSeq(certReqInfo);
		}
		{
			Asn1Sequence signatureAlgorithm;
			signatureAlgorithm.addOid(Oid::SHA256_WITH_RSA_ENCRYPTION);
			signatureAlgorithm.addNull();
			certReq.addSeq(signatureAlgorithm);
		}
		certReq.addBitString(sign<sha256>(tbs).toBinary());
		return certReq;
	}

	// Keypair

	RsaKeypair::RsaKeypair(Bigint _p, Bigint _q)
		: RsaMod(_p * _q), p(std::move(_p)), q(std::move(_q))
	{
		const auto pm1 = (p - 1_b);
		const auto qm1 = (q - 1_b);
		const auto t = (pm1 * qm1);
		if (t < RsaPublicKey::E_PREF)
		{
			SOUP_ASSERT(p > 2_b && q > 2_b);
			const auto bl = t.getBitLength();
			do
			{
				e = Bigint::randomProbablePrime(bl);
			} while (e >= t || e.isDivisorOf(t));
		}
		else
		{
			e = RsaPublicKey::E_PREF;
		}
		const auto d = e.modMulInv(t);
		dp = d.modUnsigned(pm1);
		dq = d.modUnsigned(qm1);
		qinv = q.modMulInv(p);
	}

	RsaKeypair RsaKeypair::generate(unsigned int bits, bool lax_length_requirement)
	{
		FastHardwareRngInterface rngif;
		return generate(rngif, bits, lax_length_requirement);
	}

	RsaKeypair RsaKeypair::generate(StatelessRngInterface& rng, unsigned int bits, bool lax_length_requirement)
	{
		return generate(rng, rng, bits, lax_length_requirement);
	}

	struct CaptureGenerateRng
	{
		RngInterface& rng;
		unsigned int bits;
	};

	[[nodiscard]] static Bigint gen(RngInterface& rng, unsigned int bits)
	{
		return Bigint::randomProbablePrime(rng, bits, 3);
	}

	RsaKeypair RsaKeypair::generate(RngInterface& rng, RngInterface& aux_rng, unsigned int bits, bool lax_length_requirement)
	{
		auto gen_promise = [](Capture&& _cap) -> Bigint
		{
			CaptureGenerateRng& cap = _cap.get<CaptureGenerateRng>();
			return gen(cap.rng, cap.bits);
		};

		std::vector<Bigint> primes{};
		{
#if SOUP_WASM
			primes.emplace_back(gen(rng, bits / 2u));
			primes.emplace_back(gen(aux_rng, bits / 2u));
#else
			Promise<Bigint> p(gen_promise, CaptureGenerateRng{ rng, bits / 2u });
			Promise<Bigint> q(gen_promise, CaptureGenerateRng{ aux_rng, bits / 2u });
			p.awaitFulfilment();
			q.awaitFulfilment();
			primes.emplace_back(std::move(p.getResult()));
			primes.emplace_back(std::move(q.getResult()));
#endif
		}

		bool use_aux = false;
		while (true)
		{
			for (const auto& p : primes)
			{
				for (const auto& q : primes)
				{
					if (p != q)
					{
						RsaKeypair kp(p, q);
						if (kp.n.getBitLength() == bits || lax_length_requirement)
						{
							return kp;
						}
					}
				}
			}

			if (!use_aux)
			{
				primes.emplace_back(gen(rng, bits / 2u));
			}
			else
			{
				primes.emplace_back(gen(aux_rng, bits / 2u));
			}
			use_aux = !use_aux;
		}
	}

	RsaPublicKey RsaKeypair::getPublic() const SOUP_EXCAL
	{
		return RsaPublicKey(n, e);
	}

	RsaPrivateKey RsaKeypair::getPrivate() const SOUP_EXCAL
	{
		return RsaPrivateKey(n, p, q, dp, dq, qinv);
	}
}
