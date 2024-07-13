#pragma once

#include <string>
#include <vector>

#include "rflType.hpp"
#include "rflVar.hpp"

NAMESPACE_SOUP
{
	struct rflFunc
	{
		rflType return_type;
		std::string name;
		std::vector<rflVar> parameters;

		[[nodiscard]] std::string toString() const
		{
			std::string str = return_type.toString();
			str.push_back(' ');
			str.append(name);
			str.push_back('(');
			for (auto i = parameters.begin(); i != parameters.end(); ++i)
			{
				if (i != parameters.begin())
				{
					str.append(", ");
				}
				str.append(i->toString());
			}
			str.push_back(')');
			return str;
		}
	};
}
