#pragma once

#include <vector>

#include "fwd.hpp"
#include "X509Certificate.hpp"

NAMESPACE_SOUP
{
	struct X509Certchain
	{
		std::vector<X509Certificate> certs{};

		bool fromDer(const std::vector<std::string>& vec) SOUP_EXCAL;
		bool fromPem(const std::string& str);

		void cleanup() SOUP_EXCAL;

		[[nodiscard]] bool verify(const std::string& domain, const TrustStore& ts) const SOUP_EXCAL;
		[[nodiscard]] bool verify(const TrustStore& ts) const SOUP_EXCAL;
		[[nodiscard]] bool verifyTrust(const TrustStore& ts) const SOUP_EXCAL;
		[[nodiscard]] bool isAnyInTrustStore(const TrustStore& ts) const SOUP_EXCAL;
		[[nodiscard]] bool verifySignatures() const SOUP_EXCAL;

		[[nodiscard]] std::string toString() const SOUP_EXCAL;
	};
}
