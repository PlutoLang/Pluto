#pragma once

#include <ctime>
#include <vector>

#include "fwd.hpp"

#include "Asn1Element.hpp"

namespace soup
{
	struct Asn1Sequence : public std::vector<Asn1Element>
	{
		Asn1Sequence();
		explicit Asn1Sequence(const std::string& data); // expects DER-encoded data without prefix

		[[nodiscard]] static Asn1Sequence fromDer(const std::string& str); // expects DER-encoded data with prefix
		[[nodiscard]] static Asn1Sequence fromDer(Reader& r); // expects DER-encoded data with prefix

		[[nodiscard]] size_t countChildren() const;

		[[nodiscard]] const Asn1Identifier& getChildType(const size_t child_idx) const;
		[[nodiscard]] std::string& getString(const size_t child_idx);
		[[nodiscard]] const std::string& getString(const size_t child_idx) const;
		[[nodiscard]] Asn1Sequence getSeq(const size_t child_idx) const;
		[[nodiscard]] Bigint getInt(const size_t child_idx) const;
		[[nodiscard]] Oid getOid(const size_t child_idx) const;
		[[nodiscard]] std::time_t getUtctime(const size_t child_idx) const;

		void addInt(const Bigint& val);
		void addOid(const Oid& val);
		void addNull();
		void addBitString(std::string val);
		void addUtf8String(std::string val);
		void addPrintableString(std::string val);
		void addSeq(const Asn1Sequence& seq);
		void addSet(const Asn1Sequence& seq);
		void addName(const std::vector<std::pair<Oid, std::string>>& name);
		void addUtctime(std::time_t t);

		[[nodiscard]] std::string toDer() const;
		[[nodiscard]] std::string toDerNoPrefix() const;
		[[nodiscard]] std::string toString(const std::string& prefix = {}) const;

		static Asn1Identifier readIdentifier(Reader& r);
		[[nodiscard]] static size_t readLength(Reader& r);
		[[nodiscard]] static std::string encodeLength(size_t len);
	};
}
