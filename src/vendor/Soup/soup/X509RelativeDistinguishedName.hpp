#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Oid.hpp"

namespace soup
{
	struct X509RelativeDistinguishedName : public std::vector<std::pair<Oid, std::string>>
	{
		void read(const Asn1Sequence& seq);

		[[nodiscard]] std::string get(const Oid& target) const
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

		[[nodiscard]] std::string getCommonName() const
		{
			return get(Oid::COMMON_NAME);
		}

		[[nodiscard]] std::string getCountry() const
		{
			return get({ 2, 5, 4, 6 });
		}

		[[nodiscard]] std::string getOrganisationName() const
		{
			return get({ 2, 5, 4, 10 });
		}

		[[nodiscard]] Asn1Sequence toSet() const;
	};
}
