#include "ecc.hpp"

#include "Exception.hpp"
#include "ObfusString.hpp"
#include "rand.hpp"

NAMESPACE_SOUP
{
	using namespace literals;

	[[nodiscard]] static EccCurve construct_secp256k1()
	{
		// https://asecuritysite.com/encryption/secp256k1p
		EccCurve curve;
		curve.a = Bigint{}; // 0
		curve.b = (Bigint::chunk_t)7;
		curve.p = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F"_b; // Bigint::_2pow(256) - Bigint::_2pow(32) - "977"_b
		curve.G = EccPoint{
			"0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"_b,
			"0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"_b
		};
		curve.n = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"_b;
		return curve;
	}

	[[nodiscard]] static EccCurve construct_secp256r1()
	{
		// https://asecuritysite.com/ecc/p256p
		EccCurve curve;
		curve.a = "-3"_b;
		curve.b = "41058363725152142129326129780047268409114441015993725554835256314039467401291"_b;
		curve.p = "0xFFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF"_b; // Bigint::_2pow(256) - Bigint::_2pow(224) + Bigint::_2pow(192) + Bigint::_2pow(96) - "1"_b
		curve.G = EccPoint{
			"48439561293906451759052585252797914202762949526041747995844080717082404635286"_b,
			"36134250956749795798585127919587881956611106672985015071877198253568414405109"_b
		};
		curve.n = "0xFFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551"_b;
		return curve;
	}

	[[nodiscard]] static EccCurve construct_secp384r1()
	{
		// https://asecuritysite.com/ecc/ecc_points2, https://www.secg.org/sec2-v2.pdf
		EccCurve curve;
		curve.a = "-3"_b;
		curve.b = "27580193559959705877849011840389048093056905856361568521428707301988689241309860865136260764883745107765439761230575"_b;
		curve.p = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF"_b; // Bigint::_2pow(384) - Bigint::_2pow(128) - Bigint::_2pow(96) + Bigint::_2pow(32) - "1"_b
		curve.G = EccPoint{
			"0xAA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7"_b,
			"0x3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F"_b
		};
		curve.n = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973"_b;
		return curve;
	}

	const EccCurve& EccCurve::secp256k1()
	{
		static EccCurve s_secp256k1 = construct_secp256k1();
		return s_secp256k1;
	}

	const EccCurve& EccCurve::secp256r1()
	{
		static EccCurve s_secp256r1 = construct_secp256r1();
		return s_secp256r1;
	}

	const EccCurve& EccCurve::secp384r1()
	{
		static EccCurve s_secp384r1 = construct_secp384r1();
		return s_secp384r1;
	}

	Bigint EccCurve::generatePrivate() const SOUP_EXCAL
	{
		Bigint d;
		while (d < this->n)
		{
			d.setBit(0, rand.coinflip());
			d <<= 1;
		}
		d >>= 1;
		return d;
	}

	EccPoint EccCurve::derivePublic(const Bigint& d) const
	{
		return multiply(this->G, d);
	}

	EccPoint EccCurve::add(const EccPoint& P, const EccPoint& Q) const
	{
		if (P.isPointAtInfinity())
		{
			// 0 + Q = Q
			return Q;
		}
		if (Q.isPointAtInfinity())
		{
			// P + 0 = P
			return P;
		}
		if (P.x == Q.x && P.y != Q.y)
		{
			// point1 + (-point1) = 0
			return EccPoint{};
		}
		Bigint m;
		if (P.x == Q.x)
		{
			m = (3_b * P.x.pow2() + this->a) * (2_b * P.y).modMulInv(this->p);
		}
		else
		{
			m = (P.y - Q.y) * (P.x - Q.x).modMulInv(this->p);
		}
		Bigint x = (m.pow2() - P.x - Q.x) % this->p;
		Bigint y = (m * (P.x - x) - P.y) % this->p;
		EccPoint res{ std::move(x), std::move(y) };
		//SOUP_ASSERT(validate(res));
		return res;
	}

	EccPoint EccCurve::multiply(const EccPoint& G, const Bigint& d) const
	{
#if true
		// Double-and-add
		EccPoint R;
		for (size_t i = d.getBitLength(); i-- != 0; )
		{
			R = add(R, R);
			if (d.getBitInbounds(i))
			{
				R = add(R, G);
			}
		}
		return R;
#else
		// Montgomery ladder (constant-time)
		EccPoint R0, R1 = G;
		for (int i = d.getBitLength() - 1; i >= 0; i--)
		{
			if (d.getBit(i) == 0)
			{
				R1 = add(R0, R1);
				R0 = add(R0, R0);
			}
			else
			{
				R0 = add(R0, R1);
				R1 = add(R1, R1);
			}
		}
		return R0;
#endif
	}

#undef max

	EccPoint EccCurve::multiplyAndAdd(const EccPoint& G, const Bigint& u1, const EccPoint& Q, const Bigint& u2) const
	{
		EccPoint R;

		auto l1 = u1.getBitLength();
		auto l2 = u2.getBitLength();

		auto max_len = std::max(l1, l2);

		EccPoint sum = add(G, Q);

		const EccPoint* table[3];
		table[0] = &G;
		table[1] = &Q;
		table[2] = &sum;

		for (size_t i = max_len; i-- != 0; )
		{
			R = add(R, R);

			uint8_t index = u1.getBit(i);
			index += (u2.getBit(i) * 2);
			if (index != 0)
			{
				R = add(R, *table[index - 1]);
			}
		}

		return R;
	}

	std::string EccCurve::encodePointUncompressed(const EccPoint& P) const SOUP_EXCAL
	{
		const auto bytes_per_axis = getBytesPerAxis();
		std::string str;
		str.reserve(1 + bytes_per_axis + bytes_per_axis);
		str.push_back('\x04');
		str.append(P.x.toBinary(bytes_per_axis));
		str.append(P.y.toBinary(bytes_per_axis));
		return str;
	}

	std::string EccCurve::encodePointCompressed(const EccPoint& P) const SOUP_EXCAL
	{
		const auto bytes_per_axis = getBytesPerAxis();
		std::string str;
		str.reserve(1 + bytes_per_axis);
		str.push_back(P.y.isOdd() ? '\x03' : '\x02');
		str.append(P.x.toBinary(bytes_per_axis));
		return str;
	}

	EccPoint EccCurve::decodePoint(const std::string& str) const
	{
		const auto bytes_per_axis = getBytesPerAxis();

		if (str.at(0) == 0x04)
		{
			SOUP_ASSERT(str.size() == 1 + bytes_per_axis + bytes_per_axis);
			return EccPoint{
				Bigint::fromBinary(str.substr(1, bytes_per_axis)),
				Bigint::fromBinary(str.substr(1 + bytes_per_axis, bytes_per_axis))
			};
		}

		SOUP_ASSERT(str.at(0) == 0x02 || str.at(0) == 0x03);
		const bool odd_y = (str.at(0) == 0x03);
		SOUP_ASSERT(str.size() == 1 + bytes_per_axis);
		Bigint x = Bigint::fromBinary(str.substr(1, bytes_per_axis));

		// https://github.com/mwarning/mbedtls_ecp_compression/blob/master/ecc_point_compression.c

		Bigint y = x.pow2();
		y += this->a;
		y *= x;
		y += this->b;

		Bigint n = this->p;
		++n;
		n >>= 2; // divide by 4
		y = y.modPow(n, this->p);
		if (odd_y)
		{
			y = this->p - y;
		}

		return EccPoint{ std::move(x), std::move(y) };
	}

	bool EccCurve::validate(const EccPoint& P) const SOUP_EXCAL
	{
		return (P.y.pow2() % this->p) == (((P.x * P.x * P.x) + (this->a * P.x) + this->b) % this->p);
	}

	size_t EccCurve::getBytesPerAxis() const noexcept
	{
		return p.getNumBytes();
	}

	std::pair<Bigint, Bigint> EccCurve::sign(const Bigint& d, const std::string& e) const SOUP_EXCAL
	{
		const auto z = e2z(e);

		Bigint k, r, s;
		do
		{
			do
			{
				k = Bigint::random(getBytesPerAxis()); // let's hope we never use the same k twice if we don't want to leak the d :)
				r = (multiply(G, k).x % n);
			} while (r.isZero());
			s = ((k.modMulInv(n) * (z + (r * d))) % n);
		} while (s.isZero());

		return std::pair<Bigint, Bigint>{ std::move(r), std::move(s) };
	}

	bool EccCurve::verify(const EccPoint& Q, const std::string& e, const Bigint& r, const Bigint& s) const
	{
		SOUP_IF_UNLIKELY (!validate(Q))
		{
			SOUP_THROW(Exception(ObfusString("Public key provided to EccCurve::verify is not on the curve").str()));
		}

		if (r.isNegative() || r.isZero() || r >= n
			|| s.isNegative() || s.isZero() || s >= n
			)
		{
			return false;
		}

		auto u1 = e2z(e);
		auto u2 = s.modMulInv(n);
		u1 *= u2; u1 %= n;
		u2 *= r; u2 %= n;
#if true
		// Shamir's trick
		auto p = multiplyAndAdd(G, u1, Q, u2);
#else
		auto p = multiply(G, u1);
		p = add(p, multiply(Q, u2));
#endif
		if (p.isPointAtInfinity())
		{
			return false;
		}
		p.x %= n;
		return p.x == r;
	}

	Bigint EccCurve::deriveD(const std::string& e1, const std::string& e2, const Bigint& r, const Bigint& s1, const Bigint& s2) const
	{
		const auto z1 = e2z(e1);
		const auto z2 = e2z(e2);

		const auto z_delta = ((z1 - z2) % n);
		const auto s_delta = ((s1 - s2) % n);
		const auto derived_k = z_delta.modDiv(s_delta, n);
		return ((s1 * derived_k) - z1).modDiv(r, n);
	}

	Bigint EccCurve::e2z(const std::string& e) const SOUP_EXCAL
	{
		return Bigint::fromBinary(e.substr(0, getBytesPerAxis()));
	}
}
