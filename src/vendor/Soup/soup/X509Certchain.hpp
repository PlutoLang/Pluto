#pragma once

#include <vector>

#include "fwd.hpp"
#include "X509Certificate.hpp"

namespace soup
{
	struct X509Certchain
	{
		std::vector<X509Certificate> certs{};

		bool fromDer(const std::vector<std::string>& vec);
		bool fromPem(const std::string& str);

		void cleanup();

		[[nodiscard]] bool canBeVerified() const noexcept;
		[[nodiscard]] bool verify(const std::string& domain, const TrustStore& ts) const;
		[[nodiscard]] bool verify(const TrustStore& ts) const;
		[[nodiscard]] bool verifyTrust(const TrustStore& ts) const;
		[[nodiscard]] bool isAnyInTrustStore(const TrustStore& ts) const;
		[[nodiscard]] bool verifySignatures() const;

		[[nodiscard]] std::string toString() const;
	};
}
