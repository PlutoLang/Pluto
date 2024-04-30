#include "Oid.hpp"

#include <cstring> // memcmp
#include <istream>

#include "StringRefReader.hpp"
#include "StringRefWriter.hpp"

NAMESPACE_SOUP
{
	Oid Oid::COMMON_NAME = { 2, 5, 4, 3 };
	Oid Oid::RSA_ENCRYPTION = { 1, 2, 840, 113549, 1, 1, 1 };
	Oid Oid::SHA1_WITH_RSA_ENCRYPTION = { 1, 2, 840, 113549, 1, 1, 5 };
	Oid Oid::SHA256_WITH_RSA_ENCRYPTION = { 1, 2, 840, 113549, 1, 1, 11 };
	Oid Oid::SHA384_WITH_RSA_ENCRYPTION = { 1, 2, 840, 113549, 1, 1, 12 };
	Oid Oid::SHA512_WITH_RSA_ENCRYPTION = { 1, 2, 840, 113549, 1, 1, 13 };
	Oid Oid::ECDSA_WITH_SHA256 = { 1, 2, 840, 10045, 4, 3, 2 };
	Oid Oid::ECDSA_WITH_SHA384 = { 1, 2, 840, 10045, 4, 3, 3 };
	Oid Oid::CE_KEYUSAGE = { 2, 5, 29, 15 };
	Oid Oid::CE_SUBJECTALTNAME = { 2, 5, 29, 17 };
	Oid Oid::CE_BASICCONSTRAINTS = { 2, 5, 29, 19 };
	Oid Oid::EC_PUBLIC_KEY = { 1, 2, 840, 10045, 2, 1 };
	Oid Oid::PRIME256V1 = { 1, 2, 840, 10045, 3, 1, 7 };
	Oid Oid::ANSIP384R1 = { 1, 3, 132, 0, 34 };

	Oid Oid::fromBinary(const std::string& str)
	{
		StringRefReader s{ str };
		return fromBinary(s);
	}

	Oid Oid::fromBinary(Reader& r)
	{
		Oid ret{};
		if (uint8_t first; r.u8(first))
		{
			ret.path.reserve(2);
			ret.path.push_back(first / 40);
			ret.path.push_back(first % 40);
			while (r.hasMore())
			{
				uint32_t comp;
				r.om<uint32_t>(comp);
				ret.path.emplace_back(comp);
			}
		}
		return ret;
	}

	bool Oid::operator==(const Oid& b) const noexcept
	{
		if (path.size() != b.path.size())
		{
			return false;
		}
		return memcmp(path.data(), b.path.data(), path.size() * sizeof(*path.data())) == 0;
	}

	bool Oid::operator!=(const Oid& b) const noexcept
	{
		return !operator==(b);
	}

	std::string Oid::toDer() const
	{
		std::string res;
		res.push_back((char)((path.at(0) * 40) | path.at(1)));
		StringRefWriter w(res);
		for (auto i = path.begin() + 2; i != path.end(); ++i)
		{
			w.om<uint32_t>(*i);
		}
		return res;
	}

	std::string Oid::toString() const
	{
		std::string str{};
		if (auto i = path.begin(); i != path.end())
		{
			while (true)
			{
				str.append(std::to_string(*i));
				if (++i == path.end())
				{
					break;
				}
				str.push_back('.');
			}
		}
		return str;
	}

	std::ostream& operator<<(std::ostream& os, const Oid& v)
	{
		return os << v.toString();
	}
}
