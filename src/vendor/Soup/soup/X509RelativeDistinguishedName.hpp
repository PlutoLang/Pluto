#pragma once

#include <string>
#include <utility>
#include <vector>

#include "base.hpp"
#include "Oid.hpp"

NAMESPACE_SOUP
{
	struct X509RelativeDistinguishedName : public std::vector<std::pair<Oid, std::string>>
	{
		void read(const Asn1Sequence& seq);

		[[nodiscard]] std::string get(const Oid& target) const SOUP_EXCAL
		{
			for (const auto& kv : *this)
			{
				if (kv.first == target)
				{
					return kv.second;
				}
			}
			return {};
		}

		[[nodiscard]] std::string getCommonName() const SOUP_EXCAL
		{
			return get(OID_COMMON_NAME);
		}

		[[nodiscard]] std::string getCountry() const SOUP_EXCAL
		{
			return get(Oid({ 2, 5, 4, 6 }));
		}

		[[nodiscard]] std::string getOrganisationName() const SOUP_EXCAL
		{
			return get(Oid({ 2, 5, 4, 10 }));
		}

		[[nodiscard]] Asn1Sequence toSet() const;
	};
}
