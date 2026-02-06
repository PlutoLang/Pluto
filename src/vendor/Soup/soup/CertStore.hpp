#pragma once

#include "base.hpp"

#include "rsa.hpp"
#include "X509Certchain.hpp"

NAMESPACE_SOUP
{
	struct CertStoreEntry
	{
		X509Certchain chain;
		RsaPrivateKey private_key;

		CertStoreEntry(X509Certchain&& chain, RsaPrivateKey&& private_key)
			: chain(std::move(chain)), private_key(std::move(private_key))
		{
		}
	};

	struct CertStore
	{
		std::vector<CertStoreEntry> entries{};

		void add(X509Certchain&& certchain, RsaPrivateKey&& private_key)
		{
			entries.emplace_back(std::move(certchain), std::move(private_key));
		}

		[[nodiscard]] const CertStoreEntry* findEntryForDomain(const std::string& domain) const
		{
			// In the very likely case we only have a single entry, just use that one.
			SOUP_IF_LIKELY (entries.size() == 1)
			{
				return &entries.front();
			}

			for (const auto& entry : entries)
			{
				if (entry.chain.certs.at(0).isValidForDomain(domain))
				{
					return &entry;
				}
			}
			return nullptr;
		}
	};
}
