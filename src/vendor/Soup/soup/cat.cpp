#include "cat.hpp"

#include "Reader.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
#define CAT_ASSERT(x) SOUP_IF_UNLIKELY (!(x)) { return {}; }

	UniquePtr<catNode> cat::parse(Reader& r) SOUP_EXCAL
	{
		std::string spaces;
		auto root = soup::make_unique<catNode>(nullptr);
		std::vector<catNode*> depths{ root.get() };
		size_t depth = 0;
		std::string line;
		while (r.getLine(line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			if (spaces.empty())
			{
				if (depth == 0
					&& line.c_str()[0] == ' '
					)
				{
					// Since we're just at depth 0, spaces at the beginning of a line will tell us how many spaces are supposed to equal a tab in the user's view.
					// We will then automatically convert that amount of spaces to tabs.
					size_t num_spaces = 0;
					for (const auto& c : line)
					{
						if (c != ' ')
						{
							break;
						}
						++num_spaces;
					}
					spaces = std::string(num_spaces, ' ');
					goto _convert_spaces_to_tabs;
				}
			}
			else
			{
			_convert_spaces_to_tabs:
				size_t tabs = 0;
				while (line.substr(0, spaces.length()) == spaces)
				{
					line.erase(0, spaces.length());
					++tabs;
				}
				line.insert(0, tabs, '\t');
				SOUP_IF_UNLIKELY (line.find_first_not_of('\t') == std::string::npos)
				{
					continue;
				}
			}

			// Ignore empty lines; even those with indentation.
			SOUP_IF_UNLIKELY (line.find_first_not_of('\t') == std::string::npos)
			{
				continue;
			}

			// Descend
			if (line.length() <= depth
				|| line.substr(0, depth) != std::string(depth, '\t')
				)
			{
				do
				{
					CAT_ASSERT(depth != 0);
					depths.pop_back();
					--depth;
				} while (line.length() <= depth
					|| line.substr(0, depth) != std::string(depth, '\t')
					);
			}
			else
			{
				// Ascend
				if (line.length() > depth
					&& line.substr(0, depth + 1) == std::string(depth + 1, '\t')
					)
				{
					CAT_ASSERT(!depths.at(depth)->children.empty());
					depths.emplace_back(depths.at(depth)->children.back());
					++depth;
				}
			}

			// Content
			auto node = soup::make_unique<catNode>(depths.at(depth));
			auto line_trimmed = line.substr(depth);
			size_t delim = 0;
			bool with_escaping = false;
			while (true)
			{
				delim = line_trimmed.find(':', delim);
				if (delim == std::string::npos
					|| delim == 0
					|| line_trimmed.at(delim - 1) != '\\'
					)
				{
					break;
				}
				with_escaping = true;
				++delim;
			}
			if (delim != std::string::npos)
			{
				node->name = line_trimmed.substr(0, delim);
				if (line_trimmed.size() != delim + 1)
				{
					CAT_ASSERT(line_trimmed.at(delim + 1) == ' ');
					node->value = line_trimmed.substr(delim + 2); // ": "
					SOUP_IF_UNLIKELY (node->value.size() > 2
						&& node->value.front() == '"'
						&& node->value.back() == '"'
						)
					{
						node->value.pop_back();
						node->value.erase(0, 1);
						string::replaceAll(node->value, "\\r", "\r");
						string::replaceAll(node->value, "\\n", "\n");
						string::replaceAll(node->value, "\\\"", "\"");
						string::replaceAll(node->value, "\\\\", "\\");
					}
				}
			}
			else
			{
				node->name = line_trimmed;
			}
			if (with_escaping)
			{
				string::replaceAll(node->name, "\\:", ":");
			}
			CAT_ASSERT(node->name.empty() || node->name.at(0) != '\t');
			depths.at(depth)->children.emplace_back(std::move(node));
		}
		return root;
	}

	void cat::encodeName(std::string& name)
	{
		SOUP_ASSERT(name.find("\\:") == std::string::npos);
		string::replaceAll(name, ":", "\\:");
	}

	void cat::encodeValue(std::string& value) SOUP_EXCAL
	{
		SOUP_IF_UNLIKELY (value.find_first_of("\"\n\r") != std::string::npos)
		{
			string::replaceAll(value, "\\", "\\\\");
			string::replaceAll(value, "\"", "\\\"");
			string::replaceAll(value, "\n", "\\n");
			string::replaceAll(value, "\r", "\\r");
			value.insert(0, 1, '"');
			value.push_back('"');
		}
	}
}
