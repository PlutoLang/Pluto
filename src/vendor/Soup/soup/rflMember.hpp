#pragma once

#include "rflVar.hpp"

NAMESPACE_SOUP
{
	struct rflMember : public rflVar
	{
		enum Accessibility : uint8_t
		{
			PUBLIC,
			PROTECTED,
			PRIVATE,
		};

		Accessibility accessibility;
	};
}
