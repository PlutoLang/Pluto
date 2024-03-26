#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"

namespace soup
{
	struct Oid
	{
		std::vector<uint32_t> path{};

		static Oid COMMON_NAME;
		static Oid RSA_ENCRYPTION;
		static Oid SHA256_WITH_RSA_ENCRYPTION;
		static Oid SUBJECT_ALT_NAME;
		static Oid EC_PUBLIC_KEY;
		static Oid PRIME256V1; // aka. secp256r1
		static Oid ANSIP384R1; // aka. secp384r1

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
