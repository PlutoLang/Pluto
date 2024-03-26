#pragma once

#include "Asn1Identifier.hpp"

#include <string>

namespace soup
{
	struct Asn1Element
	{
		Asn1Identifier identifier;
		std::string data;
	};
}
