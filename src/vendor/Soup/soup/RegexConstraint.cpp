#include "RegexConstraint.hpp"

#include "Regex.hpp"

NAMESPACE_SOUP
{
	void RegexConstraint::updateFlags(std::string& str, uint16_t& flags, uint16_t set, uint16_t unset) SOUP_EXCAL
	{
		set &= ~flags;
		unset &= flags;
		if (set || unset)
		{
			flags |= set;
			flags &= ~unset;
			str.append("(?");
			if (set)
			{
				Regex::unparseFlags(str, set);
			}
			if (unset)
			{
				str.push_back('-');
				Regex::unparseFlags(str, unset);
			}
			str.push_back(')');
		}
	}
}

