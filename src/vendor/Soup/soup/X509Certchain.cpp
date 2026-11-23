#include "X509Certchain.hpp"

#include <cstring> // strlen
#include <unordered_set>

#include "pem.hpp"
#include "string.hpp"
#include "TrustStore.hpp"

#define LOGGING false

#if LOGGING
#include "log.hpp"
#include "format.hpp"
#endif

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
#if LOGGING
			logWriteLine("cert invalid: expired");
#endif
			return false;
		}

		if (!certs.empty())
		{
			uint8_t max_children = 0;
			uint8_t root_index = static_cast<uint8_t>(certs.size() - 1);

			do
			{
				const auto& root = certs[root_index];
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
						if (certs.size() > 3 && root_index == certs.size() - 1)
						{
							// It's possible that the root's parent was provided, e.g. "DigiCert Global Root CA", when it's "DigiCert Global Root G2" that is the actual trust root in that case.
#if LOGGING
							logWriteLine(soup::format("root of chain is {}, but that's not a trusted root or intermediate, skipping it", root.subject.getCommonName()));
#endif
							--root_index;
							continue;
						}
#if LOGGING
						logWriteLine(soup::format("cert invalid: root of chain {} (issued by {}) is not trusted", root.subject.getCommonName(), root.issuer.getCommonName()));
#endif
						return false;
					}
					max_children = entry->max_children;
					//logWriteLine(soup::format("Allowed # of certs below {}: {}", root.issuer.getCommonName(), max_children));
					if (max_children == 0)
					{
#if LOGGING
						logWriteLine("cert invalid: intermediate has children but max_children was set to 0");
#endif
						return false;
					}
					max_children = std::min<uint8_t>(max_children - 1, root.max_children);
					if (!root.verify(*entry))
					{
#if LOGGING
						logWriteLine(soup::format("cert invalid: intermediate {} (issued by {})'s signature could not be verified against root ({})", entry->subject.getCommonName(), entry->issuer.getCommonName(), root.subject.getCommonName()));
#endif
						return false;
					}
				}
				//logWriteLine(format("Allowed # of certs below {}: {}", root.subject.getCommonName(), max_children));
				break;
			} while (true);

			// Now that we have a trusted root, we can follow the chain.
			if (certs.size() > 1)
			{
				for (auto i = certs.rbegin() + 1; i != certs.rend(); ++i)
				{
					// Handle path length constraints
					if (max_children == 0)
					{
#if LOGGING
						logWriteLine("cert invalid: too many children");
#endif
						return false;
					}
					max_children = std::min<uint8_t>(max_children - 1, i->max_children);
					//logWriteLine(format("Allowed # of certs below {}: {}", i->subject.getCommonName(), max_children));

					// Each cert must be signed by the one above it
					if (!i->verify(*(i - 1)))
					{
#if LOGGING
						logWriteLine(soup::format("cert invalid: {} (issued by {})'s signature could not be verified against {}", (i - 1)->subject.getCommonName(), (i - 1)->issuer.getCommonName(), i->subject.getCommonName()));
#endif
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
