#pragma once

#include <cstdint>
#include <string>

namespace soup
{
	struct Asn1Identifier
	{
		uint8_t m_class;
		bool constructed;
		uint32_t type;

		[[nodiscard]] std::string toDer() const;
	};
}
