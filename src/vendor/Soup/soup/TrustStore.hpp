#pragma once

#include <istream>
#include <unordered_map>

#include "X509Certificate.hpp"

NAMESPACE_SOUP
{
	class TrustStore
	{
	public:
		std::unordered_map<std::string, X509Certificate> data{};

		[[nodiscard]] static const TrustStore& fromMozilla() SOUP_EXCAL;
	private:
		[[nodiscard]] static TrustStore fromMozillaImpl() SOUP_EXCAL;

	public:
		void loadCaCerts(std::istream& is) SOUP_EXCAL; // designed for contents of cacert.pem, which can be downloaded from https://curl.se/docs/caextract.html
		void addCa(X509Certificate&& cert) SOUP_EXCAL;
		void addCa(std::string&& common_name, std::string&& pem) SOUP_EXCAL;
		void addCa(std::string&& common_name, X509Certificate&& cert) SOUP_EXCAL;

		[[nodiscard]] bool contains(const X509Certificate& cert) const SOUP_EXCAL;

		[[nodiscard]] const X509Certificate* findCommonName(const std::string& cn) const noexcept;
	};
}
