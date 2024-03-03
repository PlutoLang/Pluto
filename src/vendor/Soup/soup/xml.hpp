#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "fwd.hpp"
#include "UniquePtr.hpp"

namespace soup
{
	class xml
	{
	public:
		static XmlMode MODE_XML;
		static XmlMode MODE_LAX_XML;
		static XmlMode MODE_HTML;

		[[nodiscard]] static std::vector<UniquePtr<XmlNode>> parse(const std::string& xml, const XmlMode& mode = MODE_XML);
		[[nodiscard]] static UniquePtr<XmlTag> parseAndDiscardMetadata(const std::string& xml, const XmlMode& mode = MODE_XML);
	private:
		[[nodiscard]] static UniquePtr<XmlNode> parse(const std::string& xml, const XmlMode& mode, std::string::const_iterator& i);
	};

	struct XmlNode
	{
		const bool is_text;

		constexpr XmlNode(bool is_text) noexcept
			: is_text(is_text)
		{
		}

		virtual ~XmlNode() = default;

		[[nodiscard]] std::string encode(const XmlMode& mode = xml::MODE_XML) const noexcept;

		// Type checks.
		[[nodiscard]] bool isTag() const noexcept;
		[[nodiscard]] bool isText() const noexcept;
		
		// Type casts; will throw if node is of different type.
		[[nodiscard]] XmlTag& asTag();
		[[nodiscard]] XmlText& asText();
		[[nodiscard]] const XmlTag& asTag() const;
		[[nodiscard]] const XmlText& asText() const;
	};

	struct XmlTag : public XmlNode
	{
		std::string name{};
		std::vector<UniquePtr<XmlNode>> children{};
		std::vector<std::pair<std::string, std::string>> attributes{};

		XmlTag() noexcept
			: XmlNode(false)
		{
		}

		[[nodiscard]] std::string encode(const XmlMode& mode = xml::MODE_XML) const noexcept;

		[[nodiscard]] bool hasAttribute(const std::string& name) const noexcept;
		[[nodiscard]] const std::string& getAttribute(const std::string& name) const;

		[[nodiscard]] XmlTag* findTag(const std::string& name_target);
	};

	struct XmlText : public XmlNode
	{
		std::string contents{};

		XmlText() noexcept
			: XmlNode(true)
		{
		}

		XmlText(std::string&& contents) noexcept;

		[[nodiscard]] std::string encode() const noexcept;
	};

	struct XmlMode
	{
		// If not empty, `/>` is ignored. Instead, only contained tags are considered self-closing.
		std::unordered_set<std::string> self_closing_tags;

		// Allow attributes to be specified without a value.
		bool empty_attribute_syntax = false;
	};
}
