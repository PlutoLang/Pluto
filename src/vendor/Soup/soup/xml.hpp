#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"
#include "UniquePtr.hpp"

namespace soup
{
	struct XmlNode
	{
		const bool is_text;

		constexpr XmlNode(bool is_text) noexcept
			: is_text(is_text)
		{
		}

		virtual ~XmlNode() = default;

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

		[[nodiscard]] std::string encode() const noexcept;

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
	};

	class xml
	{
	public:
		[[nodiscard]] static std::vector<UniquePtr<XmlTag>> parse(const std::string& xml);
		[[nodiscard]] static UniquePtr<XmlTag> parseAndDiscardMetadata(const std::string& xml);
	private:
		[[nodiscard]] static UniquePtr<XmlTag> parse(const std::string& xml, std::string::const_iterator& i);
	};
}
