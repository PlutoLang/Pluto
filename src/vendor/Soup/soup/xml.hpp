#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "fwd.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	class xml
	{
	public:
		static XmlMode MODE_XML;
		static XmlMode MODE_LAX_XML;
		static XmlMode MODE_HTML;

		[[nodiscard]] static UniquePtr<XmlTag> parseAndDiscardMetadata(const std::string& xml, const XmlMode& mode = MODE_XML);
		[[nodiscard]] static UniquePtr<XmlTag> parseAndDiscardMetadata(const char* begin, const char* end, const XmlMode& mode = MODE_XML);
		[[nodiscard]] static std::vector<UniquePtr<XmlNode>> parse(const std::string& xml, const XmlMode& mode = MODE_XML);
		[[nodiscard]] static std::vector<UniquePtr<XmlNode>> parse(const char* begin, const char* end, const XmlMode& mode = MODE_XML);
	private:
		[[nodiscard]] static UniquePtr<XmlNode> parseImpl(const char*& i, const char* end, const XmlMode& mode, int max_depth);
	};

	struct XmlNode
	{
		const bool is_text;

		constexpr XmlNode(bool is_text) noexcept
			: is_text(is_text)
		{
		}

		virtual ~XmlNode() = default;

		[[nodiscard]] std::string encode(const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		[[nodiscard]] std::string encodePretty(const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		void encodeAndAppendTo(std::string& str, const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		void encodePrettyAndAppendTo(std::string& str, const XmlMode& mode = xml::MODE_XML, unsigned depth = 0) const SOUP_EXCAL;

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

		[[nodiscard]] std::string encode(const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		[[nodiscard]] std::string encodePretty(const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		void encodeAndAppendTo(std::string& str, const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;
		void encodePrettyAndAppendTo(std::string& str, const XmlMode& mode = xml::MODE_XML, unsigned depth = 0) const SOUP_EXCAL;
		void encodeAttributesAndAppendTo(std::string& str, const XmlMode& mode = xml::MODE_XML) const SOUP_EXCAL;

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

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL;
	};

	struct XmlMode
	{
		// A 0-terminated array of JOAAT Hashes. If not empty, `/>` is ignored. Instead, only contained tags are considered self-closing.
		static constexpr const uint32_t NO_SELF_CLOSING_TAGS[] = {0};
		const uint32_t* self_closing_tags = NO_SELF_CLOSING_TAGS;
		[[nodiscard]] bool hasSelfClosingTags() const noexcept { return self_closing_tags[0] != 0; }
		[[nodiscard]] bool isSelfClosingTag(const std::string& name) const noexcept;

		// Allow attributes to be specified without a value.
		bool empty_attribute_syntax = false;

		// Allow attribute values to be bare strings.
		bool unquoted_attributes = false;
	};

	inline std::string XmlNode::encode(const XmlMode& mode) const SOUP_EXCAL
	{
		std::string str;
		encodeAndAppendTo(str, mode);
		return str;
	}

	inline std::string XmlNode::encodePretty(const XmlMode& mode) const SOUP_EXCAL
	{
		std::string str;
		encodePrettyAndAppendTo(str, mode);
		return str;
	}

	inline std::string XmlTag::encode(const XmlMode& mode) const SOUP_EXCAL
	{
		std::string str;
		encodeAndAppendTo(str, mode);
		return str;
	}

	inline std::string XmlTag::encodePretty(const XmlMode& mode) const SOUP_EXCAL
	{
		std::string str;
		encodePrettyAndAppendTo(str, mode);
		return str;
	}
}
