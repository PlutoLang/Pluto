#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"

NAMESPACE_SOUP
{
	struct Oid
	{
		std::vector<uint32_t> path{};

		static const Oid COMMON_NAME;
		static const Oid RSA_ENCRYPTION;
		static const Oid SHA1_WITH_RSA_ENCRYPTION;
		static const Oid SHA256_WITH_RSA_ENCRYPTION;
		static const Oid SHA384_WITH_RSA_ENCRYPTION;
		static const Oid SHA512_WITH_RSA_ENCRYPTION;
		static const Oid ECDSA_WITH_SHA256;
		static const Oid ECDSA_WITH_SHA384;
		static const Oid CE_KEYUSAGE;
		static const Oid CE_SUBJECTALTNAME;
		static const Oid CE_BASICCONSTRAINTS;
		static const Oid EC_PUBLIC_KEY;
		static const Oid PRIME256V1; // aka. secp256r1
		static const Oid ANSIP384R1; // aka. secp384r1

		constexpr Oid() = default;

		Oid(std::initializer_list<uint32_t>&& path)
			: path(std::move(path))
		{
		}

		[[nodiscard]] static Oid fromBinary(const std::string& str);
		[[nodiscard]] static Oid fromBinary(Reader& r);

		[[nodiscard]] bool operator ==(const Oid& b) const noexcept;
		[[nodiscard]] bool operator !=(const Oid& b) const noexcept;

		[[nodiscard]] std::string toDer() const;
		[[nodiscard]] std::string toString() const;

		friend std::ostream& operator<<(std::ostream& os, const Oid& v);
	};
}
