#pragma once

#include "rflType.hpp"

NAMESPACE_SOUP
{
	struct rflVar
	{
		rflType type;
		std::string name;

		[[nodiscard]] std::string toString() const
		{
			std::string str = type.toString();
			if (!name.empty())
			{
				str.push_back(' ');
				str.append(name);
			}
			return str;
		}
	};
}
