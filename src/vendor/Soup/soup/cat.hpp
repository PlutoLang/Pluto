#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	/**
	 * Cat (short for Colons and Tabs) is a dead-simple configuration format that arose from our need for a human-friendly & easy-to-parse data exchange format.
	 * The format itself is described at https://github.com/calamity-inc/Soup/blob/senpai/docs/user/cat.md
	 * This implementation includes all optional features (value escaping, spaces).
	 */

	struct catNode
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

		virtual ~catNode() = default;
	};

	struct cat
	{
		// Returns a default-constructed UniquePtr in case of malformed data.
		[[nodiscard]] static UniquePtr<catNode> parse(Reader& r) SOUP_EXCAL;

		static void encodeName(std::string& name);
		static void encodeValue(std::string& value) SOUP_EXCAL;
	};

	[[deprecated]] inline UniquePtr<catNode> catParse(Reader& r) SOUP_EXCAL { return cat::parse(r); }
}
