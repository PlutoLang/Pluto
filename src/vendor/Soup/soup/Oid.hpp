#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"

NAMESPACE_SOUP
{
	struct Oid
	{
		static constexpr const auto MAX_SIZE = 7;

		uint8_t len = 0;
		uint8_t first;
		uint8_t second;
		uint32_t rest[MAX_SIZE - 2];

		constexpr Oid() noexcept
			: len(0), first(0), second(0), rest()
		{
		}

		template <size_t Size>
		constexpr Oid(const uint32_t(&data)[Size]) noexcept
			: len(Size), first(data[0]), second(data[1]), rest()
		{
#if SOUP_CPP20
			if (!std::is_constant_evaluated())
			{
				SOUP_ASSERT(Size <= MAX_SIZE);
			}
#endif
			for (uint32_t i = 2; i < Size; ++i)
			{
				this->rest[i - 2] = data[i];
			}
		}

		[[nodiscard]] static Oid fromBinary(const std::string& str);
		[[nodiscard]] static Oid fromBinary(Reader& r);

		[[nodiscard]] bool operator ==(const Oid& b) const noexcept;
		[[nodiscard]] bool operator !=(const Oid& b) const noexcept;

		[[nodiscard]] std::string toDer() const;
		[[nodiscard]] std::string toString() const;

		friend std::ostream& operator<<(std::ostream& os, const Oid& v);
	};

	constexpr const Oid OID_COMMON_NAME = Oid({ 2, 5, 4, 3 });
	constexpr const Oid OID_RSA_ENCRYPTION = Oid({ 1, 2, 840, 113549, 1, 1, 1 });
	constexpr const Oid OID_SHA1_WITH_RSA_ENCRYPTION = Oid({ 1, 2, 840, 113549, 1, 1, 5 });
	constexpr const Oid OID_SHA256_WITH_RSA_ENCRYPTION = Oid({ 1, 2, 840, 113549, 1, 1, 11 });
	constexpr const Oid OID_SHA384_WITH_RSA_ENCRYPTION = Oid({ 1, 2, 840, 113549, 1, 1, 12 });
	constexpr const Oid OID_SHA512_WITH_RSA_ENCRYPTION = Oid({ 1, 2, 840, 113549, 1, 1, 13 });
	constexpr const Oid OID_ECDSA_WITH_SHA256 = Oid({ 1, 2, 840, 10045, 4, 3, 2 });
	constexpr const Oid OID_ECDSA_WITH_SHA384 = Oid({ 1, 2, 840, 10045, 4, 3, 3 });
	constexpr const Oid OID_CE_KEYUSAGE = Oid({ 2, 5, 29, 15 });
	constexpr const Oid OID_CE_SUBJECTALTNAME = Oid({ 2, 5, 29, 17 });
	constexpr const Oid OID_CE_BASICCONSTRAINTS = Oid({ 2, 5, 29, 19 });
	constexpr const Oid OID_EC_PUBLIC_KEY = Oid({ 1, 2, 840, 10045, 2, 1 });
	constexpr const Oid OID_PRIME256V1 = Oid({ 1, 2, 840, 10045, 3, 1, 7 }); // aka. secp256r1
	constexpr const Oid OID_ANSIP384R1 = Oid({ 1, 3, 132, 0, 34 }); // aka. secp384r1
}
