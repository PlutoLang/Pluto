#pragma once

#include "Asn1Identifier.hpp"

#include <string>

NAMESPACE_SOUP
{
	struct Asn1Element
	{
		Asn1Identifier identifier;
		std::string data;
	};
}
