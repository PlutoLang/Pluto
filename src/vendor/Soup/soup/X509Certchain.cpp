#include "X509Certchain.hpp"

#include <cstring> // strlen
#include <unordered_set>

#include "pem.hpp"
#include "string.hpp"
#include "TrustStore.hpp"

//#include "log.hpp"
//#include "format.hpp"

NAMESPACE_SOUP
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

	bool X509Certchain::verify(const std::string& domain, const TrustStore& ts, time_t unix_timestamp) const SOUP_EXCAL
	{
		return !certs.empty()
			&& certs.at(0).isValidForDomain(domain)
			&& verify(ts, unix_timestamp)
			;
	}

	bool X509Certchain::verify(const TrustStore& ts, time_t unix_timestamp) const SOUP_EXCAL
	{
		if (certs.at(0).valid_to < unix_timestamp)
		{
			return false; // Expired
		}

		if (!certs.empty())
		{
			uint8_t max_children = 0;

			const auto& root = certs.back();
			if (ts.contains(root))
			{
				// The root of the chain is a CA cert
				max_children = root.max_children;
			}
			else
			{
				// The root of the chain is an intermediate cert
				auto entry = ts.findCommonName(root.issuer.getCommonName());
				if (!entry)
				{
					return false; // Root issuer is not in trust store
				}
				max_children = entry->max_children;
				//logWriteLine(format("Allowed # of certs below {}: {}", root.issuer.getCommonName(), max_children));
				if (max_children == 0)
				{
					return false; // Root issuer may not have any children
				}
				max_children = std::min<uint8_t>(max_children - 1, root.max_children);
				if (!root.verify(*entry))
				{
					return false;
				}
			}
			//logWriteLine(format("Allowed # of certs below {}: {}", root.subject.getCommonName(), max_children));

			// Now that we have a trusted root, we can follow the chain.
			if (certs.size() > 1)
			{
				for (auto i = certs.rbegin() + 1; i != certs.rend(); ++i)
				{
					// Handle path length constraints
					if (max_children == 0)
					{
						return false;
					}
					max_children = std::min<uint8_t>(max_children - 1, i->max_children);
					//logWriteLine(format("Allowed # of certs below {}: {}", i->subject.getCommonName(), max_children));

					// Each cert must be signed by the one above it
					if (!i->verify(*(i - 1)))
					{
						return false;
					}
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
