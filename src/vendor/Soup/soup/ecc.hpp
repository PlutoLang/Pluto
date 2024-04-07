#pragma once

#include "Bigint.hpp"

NAMESPACE_SOUP
{
	struct EccPoint
	{
		Bigint x{};
		Bigint y{};

		// aka. identity element, denoted as O.
		[[nodiscard]] bool isPointAtInfinity() const noexcept
		{
			return x.isZero() /*&& y.isZero()*/;
		}
	};

	struct EccCurve
	{
		Bigint a;
		Bigint b;
		Bigint p;
		EccPoint G;
		Bigint n;

		[[nodiscard]] static const EccCurve& secp256k1();
		[[nodiscard]] static const EccCurve& secp256r1(); // aka. P-256
		[[nodiscard]] static const EccCurve& secp384r1(); // aka. P-384

		[[nodiscard]] Bigint generatePrivate() const SOUP_EXCAL;
		[[nodiscard]] EccPoint derivePublic(const Bigint& d) const;

		[[nodiscard]] EccPoint add(const EccPoint& P, const EccPoint& Q) const;
		[[nodiscard]] EccPoint multiply(const EccPoint& G, const Bigint& d) const;
		[[nodiscard]] EccPoint multiplyAndAdd(const EccPoint& G, const Bigint& u1, const EccPoint& Q, const Bigint& u2) const;

		[[nodiscard]] std::string encodePointUncompressed(const EccPoint& P) const SOUP_EXCAL;
		[[nodiscard]] std::string encodePointCompressed(const EccPoint& P) const SOUP_EXCAL;

		[[nodiscard]] EccPoint decodePoint(const std::string& str) const;

		// Checks if P satisfies y^2 = x^3 + ax + b (mod p)
		// A valid curve point also satisfies multiply(P, n).isIdentityElement()
		[[nodiscard]] bool validate(const EccPoint& P) const SOUP_EXCAL;

		[[nodiscard]] size_t getBytesPerAxis() const noexcept;

		// ECDSA
		[[nodiscard]] std::pair<Bigint, Bigint> sign(const Bigint& d, const std::string& hash) const SOUP_EXCAL; // (r, s)
		[[nodiscard]] bool verify(const EccPoint& pub, const std::string& hash, const Bigint& r, const Bigint& s) const;
		[[nodiscard]] Bigint deriveD(const std::string& e1, const std::string& e2, const Bigint& r, const Bigint& s1, const Bigint& s2) const;
		[[nodiscard]] Bigint e2z(const std::string& e) const SOUP_EXCAL;
	};
}
