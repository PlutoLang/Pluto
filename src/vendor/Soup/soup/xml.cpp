#include "xml.hpp"

#include "Exception.hpp"
#include "string.hpp"
#include "StringBuilder.hpp"
#include "UniquePtr.hpp"

#define DEBUG_PARSE false

#if DEBUG_PARSE
#include <iostream>
#endif

namespace soup
{
	XmlMode xml::MODE_XML{};
	XmlMode xml::MODE_LAX_XML{ {}, true };
	XmlMode xml::MODE_HTML{ { "area", "base", "br", "col", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr" }, true };

	std::vector<UniquePtr<XmlNode>> xml::parse(const std::string& xml, const XmlMode& mode)
	{
		std::vector<UniquePtr<XmlNode>> res{};

		auto i = xml.begin();
		do
		{
			if (auto node = parse(xml, mode, i))
			{
				res.emplace_back(std::move(node));
			}
		} while (i != xml.end());

		return res;
	}

	UniquePtr<XmlTag> xml::parseAndDiscardMetadata(const std::string& xml, const XmlMode& mode)
	{
		auto nodes = parse(xml, mode);

		for (auto i = nodes.begin(); i != nodes.end(); )
		{
			if ((*i)->isTag())
			{
				XmlTag& tag = (*i)->asTag();
				if (tag.name.c_str()[0] == '?'
					|| tag.name.c_str()[0] == '!'
					)
				{
					i = nodes.erase(i);
					continue;
				}
			}
			++i;
		}

		if (nodes.size() == 1
			&& nodes.at(0)->isTag()
			)
		{
			SOUP_MOVE_RETURN(nodes.at(0));
		}

		auto body = soup::make_unique<XmlTag>();
		body->name = "body";
		body->children = std::move(nodes);
		return body;
	}

	UniquePtr<XmlNode> xml::parse(const std::string& xml, const XmlMode& mode, std::string::const_iterator& i)
	{
		while (i != xml.end() && string::isSpace(*i))
		{
			++i;
		}
		if (i == xml.end())
		{
			return {};
		}
		UniquePtr<XmlTag> tag;
		if (*i == '<')
		{
			++i;
			StringBuilder name_builder;
			name_builder.beginCopy(xml, i);
			for (;; ++i)
			{
				if (i == xml.end())
				{
					return tag;
				}
				if (*i == ' ' || *i == '/' || *i == '>')
				{
					break;
				}
			}
			name_builder.endCopy(xml, i);
			tag = soup::make_unique<XmlTag>();
			tag->name = std::move(name_builder);
			if (*i != '>')
			{
				// Attributes
				StringBuilder name;
				name.beginCopy(xml, i);
				for (;; ++i)
				{
					if (i == xml.end())
					{
						return tag;
					}
					if (*i == '>')
					{
						name.endCopy(xml, i);
						if (!name.empty())
						{
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
							name.clear();
						}
						break;
					}
					if (*i == ' ')
					{
						name.endCopy(xml, i);
						if (!name.empty())
						{
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
							name.clear();
						}
						name.beginCopy(xml, i + 1);
					}
					else if (*i == '/')
					{
						name.endCopy(xml, i);
						if (!name.empty())
						{
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
							name.clear();
						}
						if (mode.self_closing_tags.empty()
							&& (i + 1) != xml.end()
							&& *(i + 1) == '>'
							)
						{
#if DEBUG_PARSE
							std::cout << tag->name << " taking the easy way out" << std::endl;
#endif
							i += 2;
							return tag;
						}
						name.beginCopy(xml, i + 1);
					}
					else if (*i == '=')
					{
						name.endCopy(xml, i);
						StringBuilder value;
						++i;
						if (i != xml.end() && *i == '"')
						{
#if DEBUG_PARSE
							std::cout << "Collecting value for attribute " << name << ": ";
#endif
							++i;
							value.beginCopy(xml, i);
							for (;; ++i)
							{
								if (i == xml.end())
								{
									return tag;
								}
								if (*i == '"')
								{
									break;
								}
#if DEBUG_PARSE
								std::cout << *i;
#endif
							}
#if DEBUG_PARSE
							std::cout << std::endl;
#endif
							value.endCopy(xml, i);
							tag->attributes.emplace_back(std::move(name), std::move(value));
						}
						else if (i != xml.end() && *i == '\'')
						{
#if DEBUG_PARSE
							std::cout << "Collecting value for attribute " << name << ": ";
#endif
							++i;
							value.beginCopy(xml, i);
							for (;; ++i)
							{
								if (i == xml.end())
								{
									return tag;
								}
								if (*i == '\'')
								{
									break;
								}
#if DEBUG_PARSE
								std::cout << *i;
#endif
							}
#if DEBUG_PARSE
							std::cout << std::endl;
#endif
							value.endCopy(xml, i);
							tag->attributes.emplace_back(std::move(name), std::move(value));
						}
						else
						{
#if DEBUG_PARSE
							std::cout << "Attribute " << name << " has no value";
#endif
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
						}
						name.clear();
						name.beginCopy(xml, i + 1);
					}
				}
			}
			++i;
			if (tag->name.c_str()[0] == '?' // <?xml ... ?>
				|| tag->name.c_str()[0] == '!' // <!DOCTYPE ...>
#if SOUP_CPP20
				|| mode.self_closing_tags.contains(tag->name)
#else
				|| mode.self_closing_tags.count(tag->name)
#endif
				)
			{
				return tag; // Won't have children
			}
		}
		StringBuilder text;
		while (i != xml.end() && string::isSpace(*i))
		{
			++i;
		}
		text.beginCopy(xml, i);
		for (; i != xml.end(); ++i)
		{
			if (*i == '<')
			{
				text.endCopy(xml, i);
#if DEBUG_PARSE
				if (!text.empty())
				{
					std::cout << "Copied text: " << text << std::endl;
				}
#endif
				if (!tag)
				{
					return soup::make_unique<XmlText>(std::move(text));
				}

				if ((i + 1) != xml.end()
					&& *(i + 1) == '/'
					)
				{
					if (!text.empty())
					{
#if DEBUG_PARSE
						std::cout << "Discharging XmlText for tag close: " << text << std::endl;
#endif
						tag->children.emplace_back(soup::make_unique<XmlText>(std::move(text)));
						text.clear();
					}

					i += 2;
					StringBuilder tbc_tag;
					tbc_tag.beginCopy(xml, i);
					for (; i != xml.end(); ++i)
					{
						if (*i == '>')
						{
							break;
						}
					}
					tbc_tag.endCopy(xml, i);
					++i;
#if DEBUG_PARSE
					std::cout << "tbc tag: " << tbc_tag << std::endl;
#endif
					if (tbc_tag != tag->name)
					{
#if DEBUG_PARSE
						std::cout << "Expected tbc tag to be " << tag->name << std::endl;
#endif
						i -= tbc_tag.length();
						i -= 3;
#if DEBUG_PARSE
						std::cout << "Cursor now at " << *i << ", unwinding" << std::endl;
#endif
						return tag;
					}
					text.beginCopy(xml, i);
					break;
				}

				// Handle CDATA sections
				if ((i + 1) != xml.end() && *(i + 1) == '!'
					&& (i + 2) != xml.end() && *(i + 2) == '['
					&& (i + 3) != xml.end() && *(i + 3) == 'C'
					&& (i + 4) != xml.end() && *(i + 4) == 'D'
					&& (i + 5) != xml.end() && *(i + 5) == 'A'
					&& (i + 6) != xml.end() && *(i + 6) == 'T'
					&& (i + 7) != xml.end() && *(i + 7) == 'A'
					&& (i + 8) != xml.end() && *(i + 8) == '['
					)
				{
					i += 9;
					text.beginCopy(xml, i);
					for (; i != xml.end(); ++i)
					{
						if (*i == ']'
							&& (i + 1) != xml.end() && *(i + 1) == ']'
							&& (i + 2) != xml.end() && *(i + 2) == '>'
							)
						{
							text.endCopy(xml, i);
							i += 3;
							break;
						}
					}
#if DEBUG_PARSE
					if (!text.empty())
					{
						std::cout << "Copied text from CDATA section: " << text << std::endl;
					}
#endif
					text.beginCopy(xml, i);
					--i; // Cursor is already in the right place but for loop will do `++i`
					continue;
				}

				if (!text.empty())
				{
#if DEBUG_PARSE
					std::cout << "Discharging XmlText for nested tag: " << text << std::endl;
#endif
					tag->children.emplace_back(soup::make_unique<XmlText>(std::move(text)));
					text.clear();
				}
#if DEBUG_PARSE
				auto child = parse(xml, mode, i);
				std::cout << "Recursed for " << child->encode() << std::endl;
				tag->children.emplace_back(std::move(child));
#else
				tag->children.emplace_back(parse(xml, mode, i));
#endif
				if (i == xml.end())
				{
					text.beginCopy(xml, i);
					break;
				}
				while (string::isSpace(*i) && ++i != xml.end());
				text.beginCopy(xml, i);
				--i; // Cursor is already in the right place but for loop will do `++i`
			}
		}
		text.endCopy(xml, i);
		if (!tag)
		{
			return soup::make_unique<XmlText>(std::move(text));
		}
		if (!text.empty())
		{
#if DEBUG_PARSE
			std::cout << "Discharging XmlText for return: " << text << std::endl;
#endif
			tag->children.emplace_back(soup::make_unique<XmlText>(std::move(text)));
			text.clear();
		}
		return tag;
	}

	std::string XmlNode::encode(const XmlMode& mode) const noexcept
	{
		if (is_text)
		{
			return static_cast<const XmlText*>(this)->encode();
		}
		return static_cast<const XmlTag*>(this)->encode(mode);
	}

	bool XmlNode::isTag() const noexcept
	{
		return !is_text;
	}

	bool XmlNode::isText() const noexcept
	{
		return is_text;
	}

	XmlTag& XmlNode::asTag()
	{
		if (!isTag())
		{
			SOUP_THROW(Exception("XmlNode has unexpected type"));
		}
		return *static_cast<XmlTag*>(this);
	}

	XmlText& XmlNode::asText()
	{
		if (!isText())
		{
			SOUP_THROW(Exception("XmlNode has unexpected type"));
		}
		return *static_cast<XmlText*>(this);
	}

	const XmlTag& XmlNode::asTag() const
	{
		if (!isTag())
		{
			SOUP_THROW(Exception("XmlNode has unexpected type"));
		}
		return *static_cast<const XmlTag*>(this);
	}

	const XmlText& XmlNode::asText() const
	{
		if (!isText())
		{
			SOUP_THROW(Exception("XmlNode has unexpected type"));
		}
		return *static_cast<const XmlText*>(this);
	}

	std::string XmlTag::encode(const XmlMode& mode) const noexcept
	{
#if SOUP_CPP20
		const bool is_self_closing = mode.self_closing_tags.contains(name);
#else
		const bool is_self_closing = mode.self_closing_tags.count(name);
#endif

		std::string str(1, '<');
		str.append(name);
		for (const auto& e : attributes)
		{
			str.push_back(' ');
			//str.push_back('{');
			str.append(e.first);
			//str.push_back('}');
			if (!e.second.empty()
				|| !mode.empty_attribute_syntax
				)
			{
				str.push_back('=');
				if (e.second.find('"') != std::string::npos)
				{
					str.push_back('\'');
					str.append(e.second);
					str.push_back('\'');
				}
				else
				{
					str.push_back('"');
					str.append(e.second);
					str.push_back('"');
				}
			}
		}
		if (is_self_closing)
		{
			str.append(" /");
		}
		str.push_back('>');
		for (const auto& child : children)
		{
			str.append(child->encode(mode));
		}
		if (!is_self_closing)
		{
			str.append("</");
			str.append(name);
			str.push_back('>');
		}
		return str;
	}

	bool XmlTag::hasAttribute(const std::string& name) const noexcept
	{
		for (const auto& a : attributes)
		{
			if (a.first == name)
			{
				return true;
			}
		}
		return false;
	}

	const std::string& XmlTag::getAttribute(const std::string& name) const
	{
		for (const auto& a : attributes)
		{
			if (a.first == name)
			{
				return a.second;
			}
		}
		SOUP_ASSERT_UNREACHABLE;
	}

	XmlTag* XmlTag::findTag(const std::string& name_target)
	{
		if (name == name_target)
		{
			return this;
		}
		XmlTag* res = nullptr;
		for (const auto& child : children)
		{
			if (!child->is_text)
			{
				res = static_cast<XmlTag*>(child.get())->findTag(name_target);
				if (res != nullptr)
				{
					break;
				}
			}
		}
		return res;
	}

	XmlText::XmlText(std::string&& contents) noexcept
		: XmlNode(true), contents(std::move(contents))
	{
		string::replaceAll(this->contents, "&amp;", "&");
		string::replaceAll(this->contents, "&lt;", "<");
		string::replaceAll(this->contents, "&gt;", ">");
	}

	std::string XmlText::encode() const noexcept
	{
		std::string contents = this->contents;
		string::replaceAll(contents, "&", "&amp;");
		string::replaceAll(contents, "<", "&lt;");
		string::replaceAll(contents, ">", "&gt;");
		return contents;
	}
}
