#pragma once

#include <optional>
#include <vector>

#include "RegexMatchedGroup.hpp"

NAMESPACE_SOUP
{
	struct RegexMatchResult
	{
		std::vector<std::optional<RegexMatchedGroup>> groups{};

		[[nodiscard]] bool isSuccess() const noexcept
		{
			return !groups.empty();
		}

		[[nodiscard]] size_t length() const
		{
			return groups.at(0).value().length();
		}

		[[nodiscard]] const RegexMatchedGroup* findGroupByIndex(size_t i) const noexcept
		{
			if (i < groups.size()
				&& groups.at(i).has_value()
				)
			{
				return &groups.at(i).value();
			}
			return nullptr;
		}

		[[nodiscard]] const RegexMatchedGroup* findGroupByName(const std::string& name) const noexcept
		{
			for (size_t i = 0; i != groups.size(); ++i)
			{
				if (groups.at(i).has_value()
					&& groups.at(i)->name == name
					)
				{
					return &groups.at(i).value();
				}
			}
			return nullptr;
		}

		[[nodiscard]] std::string toString() const noexcept
		{
			std::string str{};
			for (size_t i = 0; i != groups.size(); ++i)
			{
				if (groups.at(i).has_value())
				{
					str.append(std::to_string(i));
					if (!groups.at(i)->name.empty())
					{
						str.push_back('{');
						str.append(groups.at(i)->name);
						str.push_back('}');
					}
					str.append("=\"");
					str.append(groups.at(i)->toString());
					str.append("\", ");
				}
			}
			if (!str.empty())
			{
				str.erase(str.size() - 2, 2);
			}
			return str;
		}
	};
}
