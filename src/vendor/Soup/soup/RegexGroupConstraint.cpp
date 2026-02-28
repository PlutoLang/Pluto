#include "RegexGroupConstraint.hpp"

#include "Regex.hpp"

NAMESPACE_SOUP
{
	void RegexGroupConstraint::toString(std::string& str, uint16_t& flags) const SOUP_EXCAL
	{
		str.push_back('(');
		if (data.isNonCapturing())
		{
			str.push_back('?');
			if (uint16_t set = data.initial_flags & ~flags)
			{
				flags |= set;
				Regex::unparseFlags(str, set);
			}
			str.push_back(':');
		}
		else
		{
			str.push_back('?');
			if (data.name.find('\'') != std::string::npos)
			{
				str.push_back('<');
				str.append(data.name);
				str.push_back('>');
			}
			else
			{
				str.push_back('\'');
				str.append(data.name);
				str.push_back('\'');
			}
		}
		data.toString(str, flags);
		str.push_back(')');
	}
}
