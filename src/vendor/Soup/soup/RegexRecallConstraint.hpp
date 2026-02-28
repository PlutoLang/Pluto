#pragma once

#include "RegexConstraint.hpp"

NAMESPACE_SOUP
{
	struct RegexRecallConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matchesImpl(RegexMatcher& m, const RegexMatchedGroup* group) const noexcept
		{
			if (group)
			{
				auto it = m.it;
				for (auto group_it = group->begin; group_it != group->end; ++group_it)
				{
					if (it == m.end
						|| *it != *group_it
						)
					{
						return false;
					}
					++it;
				}
				m.it = it;
				return true;
			}
			return false;
		}
	};

	struct RegexRecallIndexConstraint : public RegexRecallConstraint
	{
		const size_t i;

		RegexRecallIndexConstraint(size_t i)
			: i(i)
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return matchesImpl(m, m.result.findGroupByIndex(i));
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			str.push_back('\\');
			str.append(std::to_string(i));
		}
	};

	struct RegexRecallNameConstraint : public RegexRecallConstraint
	{
		const std::string name;

		RegexRecallNameConstraint(std::string&& name)
			: name(std::move(name))
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return matchesImpl(m, m.result.findGroupByName(name));
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			str.append("\\k");
			if (name.find('\'') != std::string::npos)
			{
				str.push_back('<');
				str.append(name);
				str.push_back('>');
			}
			else
			{
				str.push_back('\'');
				str.append(name);
				str.push_back('\'');
			}
		}
	};
}
