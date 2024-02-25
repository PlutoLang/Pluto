#include "X509Certchain.hpp"

#include <cstring> // strlen
#include <unordered_set>

#include "pem.hpp"
#include "string.hpp"
#include "TrustStore.hpp"

namespace soup
{
	bool X509Certchain::fromDer(const std::vector<std::string>& vec) SOUP_EXCAL
	{
		for (auto& cert : vec)
		{
			X509Certificate xcert{};
			if (!xcert.fromDer(cert))
			{
				return false;
			}
			certs.emplace_back(std::move(xcert));
		}
		return !certs.empty();
	}

	bool X509Certchain::fromPem(const std::string& str)
	{
		for (const auto& der : pem::decodeChain(str))
		{
			X509Certificate xcert{};
			if (!xcert.fromDer(der))
			{
				return false;
			}
			certs.emplace_back(std::move(xcert));
		}
		return !certs.empty();
	}

	void X509Certchain::cleanup() SOUP_EXCAL
	{
		// We assume that each entry in the certchain is signed by the next,
		// but a server may be misconfigured and provide the subject certificate multiple times,
		// as is the case with content.warframe.com at the time of writing this.
		std::unordered_set<uint32_t> seen_before;
		for (auto i = certs.begin(); i != certs.end(); )
		{
#if SOUP_CPP20
			if (seen_before.contains(i->hash))
#else
			if (seen_before.find(i->hash) != seen_before.end())
#endif
			{
				i = certs.erase(i);
			}
			else
			{
				seen_before.emplace(i->hash);
				++i;
			}
		}
	}

	bool X509Certchain::verify(const std::string& domain, const TrustStore& ts) const SOUP_EXCAL
	{
		return certs.at(0).isValidForDomain(domain)
			&& verify(ts)
			;
	}

	bool X509Certchain::verify(const TrustStore& ts) const SOUP_EXCAL
	{
		return verifyTrust(ts)
			&& verifySignatures()
			;
	}

	bool X509Certchain::verifyTrust(const TrustStore& ts) const SOUP_EXCAL
	{
		if (!certs.empty())
		{
			if (isAnyInTrustStore(ts))
			{
				return true;
			}

			const auto& root = certs.back();
			if (auto entry = ts.findCommonName(root.issuer.getCommonName()))
			{
				if (root.verify(*entry))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool X509Certchain::isAnyInTrustStore(const TrustStore& ts) const SOUP_EXCAL
	{
		for (auto i = certs.rbegin(); i != certs.rend(); ++i)
		{
			if (auto entry = ts.findCommonName(i->subject.getCommonName()))
			{
				if (entry->isEc() == i->isEc()
					&& entry->key.x == i->key.x
					&& entry->key.y == i->key.y
					)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool X509Certchain::verifySignatures() const SOUP_EXCAL
	{
		if (certs.size() > 1)
		{
			for (auto i = certs.begin(); i != certs.end() - 1; ++i)
			{
				if (!i->verify(*(i + 1)))
				{
					return false;
				}
			}
		}
		return true;
	}

	std::string X509Certchain::toString() const SOUP_EXCAL
	{
		std::string str{};
		if (!certs.empty())
		{
			for (const auto& cert : certs)
			{
				str.append(cert.subject.getCommonName());
				str.append(" < ");
			}
			str.append(certs.back().issuer.getCommonName());
		}
		return str;
	}
}
