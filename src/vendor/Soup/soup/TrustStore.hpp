#pragma once

#include <istream>
#include <unordered_map>

#include "X509Certificate.hpp"

namespace soup
{
	struct TrustStore
	{
		std::unordered_map<std::string, X509Certificate> data{};

		[[nodiscard]] static TrustStore fromMozilla();

		void loadCaCerts(std::istream& is); // designed for contents of cacert.pem, which can be downloaded from https://curl.se/docs/caextract.html
		void addCa(X509Certificate&& cert);
		void addCa(std::string&& common_name, std::string&& pem);
		void addCa(std::string&& common_name, X509Certificate&& cert);

		[[nodiscard]] const X509Certificate* findCommonName(const std::string& cn) const;
	};
}
