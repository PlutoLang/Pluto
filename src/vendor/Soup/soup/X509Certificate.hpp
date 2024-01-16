#pragma once

#include <ctime>

#include "Asn1Sequence.hpp"
#include "ecc.hpp"
#include "rsa.hpp"
#include "X509RelativeDistinguishedName.hpp"

namespace soup
{
	struct X509Certificate
	{
		enum SigType : uint8_t
		{
			UNK_WITH_UNK,
			RSA_WITH_SHA1,
			RSA_WITH_SHA256,
			RSA_WITH_SHA384,
			RSA_WITH_SHA512,
			ECDSA_WITH_SHA256,
			ECDSA_WITH_SHA384,
		};

		std::string tbsCertDer;
		SigType sig_type;
		std::string sig;

		uint32_t hash;
		X509RelativeDistinguishedName issuer;
		X509RelativeDistinguishedName subject;
		std::vector<std::string> subject_alt_names;

		bool is_ec;
		EccPoint key;
		const EccCurve* curve;

		std::time_t valid_from;
		std::time_t valid_to;

		bool fromDer(const std::string& str) noexcept;
		bool load(const Asn1Sequence& cert) noexcept;

		[[nodiscard]] bool isRsa() const noexcept { return !is_ec; }
		[[nodiscard]] bool isEc() const noexcept { return is_ec; }
		[[nodiscard]] RsaPublicKey getRsaPublicKey() const SOUP_EXCAL;
		void setRsaPublicKey(Bigint n, Bigint e) noexcept;
		void setRsaPublicKey(RsaPublicKey pub) noexcept;

		[[nodiscard]] bool canBeVerified() const noexcept;
		[[nodiscard]] bool verify(const X509Certificate& issuer) const;

		[[nodiscard]] bool isValidForDomain(const std::string& domain) const SOUP_EXCAL;
		[[nodiscard]] static bool matchDomain(const std::string& domain, const std::string& name) SOUP_EXCAL;

		// Does not generate valid certificates by most opinions.
		[[nodiscard]] Asn1Sequence toAsn1() const SOUP_EXCAL;
		[[nodiscard]] std::string toDer() const SOUP_EXCAL;
	};
}
