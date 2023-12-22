#include "X509Certificate.hpp"

#include <cstring> // memcmp

#include "Asn1Type.hpp"
#include "IpAddr.hpp"
#include "joaat.hpp"
#include "sha1.hpp"
#include "sha256.hpp"
#include "string.hpp"

namespace soup
{
	bool X509Certificate::fromDer(const std::string& str)
	{
		return load(Asn1Sequence::fromDer(str));
	}

	bool X509Certificate::load(const Asn1Sequence& cert) noexcept
	{
#if SOUP_EXCEPTIONS
		try
#endif
		{
			auto tbsCert = cert.getSeq(0);
			switch (joaat::hash(cert.getSeq(1).getOid(0).toString()))
			{
			default:
				sig_type = UNK_WITH_UNK;
				break;

			case joaat::hash("1.2.840.113549.1.1.5"):
				sig_type = RSA_WITH_SHA1;
				break;

			case joaat::hash("1.2.840.113549.1.1.11"):
				sig_type = RSA_WITH_SHA256;
				break;

			case joaat::hash("1.2.840.113549.1.1.12"):
				sig_type = RSA_WITH_SHA384;
				break;

			case joaat::hash("1.2.840.113549.1.1.13"):
				sig_type = RSA_WITH_SHA512;
				break;

			case joaat::hash("1.2.840.10045.4.3.2"):
				sig_type = ECDSA_WITH_SHA256;
				break;

			case joaat::hash("1.2.840.10045.4.3.3"):
				sig_type = ECDSA_WITH_SHA384;
				break;
			}
			tbsCertDer = tbsCert.toDer();
			sig = cert.getString(2);
			sig.erase(0, 1); // trim leading zero

			hash = joaat::hash(cert.at(0).data);

			auto pubInfo = tbsCert.getSeq(6);
			auto pubType = pubInfo.getSeq(0);
			auto pubCrypto = pubType.getOid(0);

			std::string pubKeyStr = pubInfo.getString(1);
			pubKeyStr.erase(0, 1); // trim leading zero

			if (pubCrypto == Oid::RSA_ENCRYPTION)
			{
				is_ec = false;

				auto pubKey = Asn1Sequence::fromDer(pubKeyStr);
				setRsaPublicKey(
					pubKey.getInt(0),
					pubKey.getInt(1)
				);
			}
			else if (pubCrypto == Oid::EC_PUBLIC_KEY)
			{
				is_ec = true;

				auto pubCurve = pubType.getOid(1);
				if (pubCurve == Oid::PRIME256V1)
				{
					curve = &EccCurve::secp256r1();
				}
				else if (pubCurve == Oid::ANSIP384R1)
				{
					curve = &EccCurve::secp384r1();
				}
				else
				{
					curve = nullptr;
				}

				if (curve)
				{
					key = curve->decodePoint(pubKeyStr);
				}
			}
			else
			{
				return false;
			}

			issuer.read(tbsCert.getSeq(3));
			subject.read(tbsCert.getSeq(5));

			Asn1Sequence validityPeriod = tbsCert.getSeq(4);
			valid_from = validityPeriod.getUtctime(0);
			valid_to = validityPeriod.getUtctime(1);

			Asn1Sequence extensions = tbsCert.getSeq(7).getSeq(0);
			for (size_t i = 0; i != extensions.countChildren(); ++i)
			{
				Asn1Sequence ext = extensions.getSeq(i);
				if (ext.getOid(0) == Oid::SUBJECT_ALT_NAME)
				{
					size_t data_idx = ((ext.at(1).identifier.type == ASN1_BOOLEAN) ? 2 : 1);
					// RFC 2459, page 33
					Asn1Sequence data = Asn1Sequence::fromDer(ext.getString(data_idx));
					for (size_t j = 0; j != data.countChildren(); ++j)
					{
						if (data.getChildType(j).type == 2) // DNS Name
						{
							subject_alt_names.emplace_back(std::move(data.getString(j)));
						}
						else if (data.getChildType(j).type == 7) // IP Address
						{
							if (data.getString(j).size() == 4)
							{
								subject_alt_names.emplace_back(IpAddr(*(network_u32_t*)data.getString(j).data()).toString4());
							}
							else if (data.getString(j).size() == 16)
							{
								subject_alt_names.emplace_back(IpAddr((uint8_t*)data.getString(j).data()).toString6());
							}
						}
					}
				}
			}

			return true;
		}
#if SOUP_EXCEPTIONS
		catch (const std::out_of_range&)
		{
		}
#endif
		return false;
	}

	void X509Certificate::setRsaPublicKey(Bigint n, Bigint e)
	{
		key = EccPoint{ std::move(n), std::move(e) };
	}

	void X509Certificate::setRsaPublicKey(RsaPublicKey pub)
	{
		key = EccPoint{ std::move(pub.n), std::move(pub.e) };
	}

	RsaPublicKey X509Certificate::getRsaPublicKey() const
	{
		return RsaPublicKey(key.x, key.y);
	}

	bool X509Certificate::canBeVerified() const noexcept
	{
		switch (sig_type)
		{
		case RSA_WITH_SHA1:
		case RSA_WITH_SHA256:
		case ECDSA_WITH_SHA256:
			return true;

		default:;
		}
		return false;
	}

	bool X509Certificate::verify(const X509Certificate& issuer) const
	{
		switch (sig_type)
		{
		case RSA_WITH_SHA1:
			if (!issuer.isRsa())
			{
				return false;
			}
			return issuer.getRsaPublicKey().verify<soup::sha1>(tbsCertDer, Bigint::fromBinary(sig));

		case RSA_WITH_SHA256:
			if (!issuer.isRsa())
			{
				return false;
			}
			return issuer.getRsaPublicKey().verify<soup::sha256>(tbsCertDer, Bigint::fromBinary(sig));

		case ECDSA_WITH_SHA256:
			if (!issuer.isEc()
				|| !issuer.curve
				)
			{
				return false;
			}
			{
				auto seq = Asn1Sequence(sig).getSeq(0);
				auto r = seq.getInt(0);
				auto s = seq.getInt(1);
				return issuer.curve->verify(issuer.key, sha256::hash(tbsCertDer), r, s);
			}

			// TODO: Implement SHA384 & SHA512
		case RSA_WITH_SHA384: return issuer.isRsa();
		case RSA_WITH_SHA512: return issuer.isRsa();
		case ECDSA_WITH_SHA384: return issuer.isEc();

		case UNK_WITH_UNK:;
		}
		return false;
	}

	bool X509Certificate::isValidForDomain(const std::string& domain) const
	{
		if (matchDomain(domain, subject.getCommonName()))
		{
			return true;
		}
		for (const auto& name : subject_alt_names)
		{
			if (matchDomain(domain, name))
			{
				return true;
			}
		}
		return false;
	}

	bool X509Certificate::matchDomain(const std::string& domain, const std::string& name)
	{
		auto domain_parts = string::explode(domain, '.');
		auto name_parts = string::explode(name, '.');
		if (domain_parts.size() != name_parts.size())
		{
			return false;
		}
		for (size_t i = 0; i != domain_parts.size(); ++i)
		{
			if (name_parts.at(i) != "*"
				&& name_parts.at(i) != domain_parts.at(i)
				)
			{
				return false;
			}
		}
		return true;
	}

	Asn1Sequence X509Certificate::toAsn1() const
	{
		Asn1Sequence algo_seq;
		algo_seq.addOid(Oid::SHA256_WITH_RSA_ENCRYPTION); // RSA_WITH_SHA256
		algo_seq.addNull();

		//auto tbsCert = Asn1Sequence::fromDer(tbsCertDer);
		Asn1Sequence tbsCert;
		tbsCert.addNull(); // [0] This should be "Version 3", respresented by "Constructed Context-specific 0: 020102"
		tbsCert.addInt({}); // [1] Serial number
		tbsCert.addSeq(algo_seq);
		tbsCert.addSet(issuer.toSet());
		{
			Asn1Sequence validityPeriod;
			validityPeriod.addUtctime(valid_from);
			validityPeriod.addUtctime(valid_to);
			tbsCert.addSeq(validityPeriod);
		}
		tbsCert.addSet(subject.toSet());
		{
			Asn1Sequence pubInfo;
			{
				Asn1Sequence pubType;
				pubType.addOid(Oid::RSA_ENCRYPTION);
				pubType.addNull();
				pubInfo.addSeq(pubType);
			}
			{
				auto key = getRsaPublicKey();

				Asn1Sequence pubKey;
				pubKey.addInt(key.n);
				pubKey.addInt(key.e);
				pubInfo.addBitString(pubKey.toDer());
			}
			tbsCert.addSeq(pubInfo);
		}
		{
			Asn1Sequence dummy;
			dummy.addSeq({});
			tbsCert.addSeq(dummy); // [7] Extensions, should be a context-specific 3.
		}

		Asn1Sequence cert;
		cert.addSeq(tbsCert);
		cert.addSeq(algo_seq);
		cert.addBitString(sig);
		return cert;
	}

	std::string X509Certificate::toDer() const
	{
		return toAsn1().toDer();
	}
}
