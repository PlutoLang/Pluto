#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"
#include "TreeNode.hpp"

#include "UniquePtr.hpp"

namespace soup
{
	/**
	 * Cat (short for Colons and Tabs) is a format that arose from our need for a simple data exchange format that is human-readable and maybe even human-writable.
	 * People would usually use JSON for that task, but let's not kid ourselves, JSON is not very human-friendly, as I've seen first-hand.
	 * JSON also has the drawback of being rather difficult to parse, so you'd be crawling to rapidjson, nlohmann/json, or whatever.
	 * Cat is dead-simple to write from any existing tree structure and a parser can be implemented in about 50 lines of code.
	 * Strings are the only value type because they can be easily converted to and from bool, int, float, and even custom types.
	 */

	struct catNode : public TreeNode
	{
		catNode* parent;
		std::string name;
		std::string value;
		std::vector<UniquePtr<catNode>> children{};

		catNode(catNode* parent)
			: parent(parent)
		{
		}

		catNode(catNode* parent, std::string name, std::string value)
			: parent(parent), name(std::move(name)), value(std::move(value))
		{
		}
	};

	// Returns a default-constructed UniquePtr in case of malformed data.
	[[nodiscard]] UniquePtr<catNode> catParse(Reader& r) noexcept;
}
