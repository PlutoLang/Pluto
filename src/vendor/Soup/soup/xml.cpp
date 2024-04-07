#include "xml.hpp"

#include "Exception.hpp"
#include "string.hpp"
#include "StringBuilder.hpp"
#include "UniquePtr.hpp"

#define DEBUG_PARSE false

#if DEBUG_PARSE
#include <iostream>
#endif

NAMESPACE_SOUP
{
	XmlMode xml::MODE_XML{};
	XmlMode xml::MODE_LAX_XML{ {}, true, true };
	XmlMode xml::MODE_HTML{ { "area", "base", "br", "col", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr" }, true, true };

	UniquePtr<XmlTag> xml::parseAndDiscardMetadata(const std::string& xml, const XmlMode& mode)
	{
		return parseAndDiscardMetadata(xml.data(), &xml.data()[xml.size()], mode);
	}

	UniquePtr<XmlTag> xml::parseAndDiscardMetadata(const char* begin, const char* end, const XmlMode& mode)
	{
		auto nodes = parse(begin, end, mode);

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

	std::vector<UniquePtr<XmlNode>> xml::parse(const std::string& xml, const XmlMode& mode)
	{
		return parse(xml.data(), &xml.data()[xml.size()], mode);
	}

	std::vector<UniquePtr<XmlNode>> xml::parse(const char* begin, const char* end, const XmlMode& mode)
	{
		std::vector<UniquePtr<XmlNode>> res{};

		auto i = begin;
		do
		{
			if (auto node = parseImpl(i, end, mode))
			{
				res.emplace_back(std::move(node));
			}
		} while (i != end);

		return res;
	}

	UniquePtr<XmlNode> xml::parseImpl(const char*& i, const char* end, const XmlMode& mode)
	{
		while (i != end && string::isSpace(*i))
		{
			++i;
		}
		if (i == end)
		{
			return {};
		}
		UniquePtr<XmlTag> tag;
		if (*i == '<')
		{
			++i;
			StringBuilder name_builder;
			name_builder.beginCopy(i);
			for (;; ++i)
			{
				if (i == end)
				{
					return tag;
				}
				if (*i == ' ' || *i == '/' || *i == '>')
				{
					break;
				}
			}
			name_builder.endCopy(i);
			tag = soup::make_unique<XmlTag>();
			tag->name = std::move(name_builder);
			if (*i != '>')
			{
				// Attributes
				StringBuilder name;
				name.beginCopy(i);
				for (;; ++i)
				{
					if (i == end)
					{
						return tag;
					}
					if (*i == '>')
					{
						name.endCopy(i);
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
						name.endCopy(i);
						if (!name.empty())
						{
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
							name.clear();
						}
						name.beginCopy(i + 1);
					}
					else if (*i == '/')
					{
						name.endCopy(i);
						if (!name.empty())
						{
							if (mode.empty_attribute_syntax)
							{
								tag->attributes.emplace_back(std::move(name), std::string());
							}
							name.clear();
						}
						if (mode.self_closing_tags.empty()
							&& (i + 1) != end
							&& *(i + 1) == '>'
							)
						{
#if DEBUG_PARSE
							std::cout << tag->name << " taking the easy way out" << std::endl;
#endif
							i += 2;
							return tag;
						}
						name.beginCopy(i + 1);
					}
					else if (*i == '=')
					{
						name.endCopy(i);
						StringBuilder value;
						++i;
						if (i != end && *i == '"')
						{
#if DEBUG_PARSE
							std::cout << "Collecting value for attribute " << name << ": ";
#endif
							++i;
							value.beginCopy(i);
							for (;; ++i)
							{
								if (i == end)
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
							value.endCopy(i);
							tag->attributes.emplace_back(std::move(name), std::move(value));
						}
						else if (i != end && *i == '\'')
						{
#if DEBUG_PARSE
							std::cout << "Collecting value for attribute " << name << ": ";
#endif
							++i;
							value.beginCopy(i);
							for (;; ++i)
							{
								if (i == end)
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
							value.endCopy(i);
							tag->attributes.emplace_back(std::move(name), std::move(value));
						}
						else if (i != end && string::isAlphaNum(*i) && mode.unquoted_attributes)
						{
#if DEBUG_PARSE
							std::cout << "Collecting value for attribute " << name << ": ";
#endif
							value.beginCopy(i);
							for (;; ++i)
							{
								if (i == end)
								{
									return tag;
								}
								if (!string::isAlphaNum(*i))
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
							value.endCopy(i);
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
						name.beginCopy(i + 1);
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
		while (i != end && string::isSpace(*i))
		{
			++i;
		}
		text.beginCopy(i);
		for (; i != end; ++i)
		{
			if (*i == '<')
			{
				text.endCopy(i);
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

				if ((i + 1) != end
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
					tbc_tag.beginCopy(i);
					for (; i != end; ++i)
					{
						if (*i == '>')
						{
							break;
						}
					}
					tbc_tag.endCopy(i);
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
					text.beginCopy(i);
					break;
				}

				// Handle CDATA sections
				if ((i + 1) != end && *(i + 1) == '!'
					&& (i + 2) != end && *(i + 2) == '['
					&& (i + 3) != end && *(i + 3) == 'C'
					&& (i + 4) != end && *(i + 4) == 'D'
					&& (i + 5) != end && *(i + 5) == 'A'
					&& (i + 6) != end && *(i + 6) == 'T'
					&& (i + 7) != end && *(i + 7) == 'A'
					&& (i + 8) != end && *(i + 8) == '['
					)
				{
					i += 9;
					text.beginCopy(i);
					for (; i != end; ++i)
					{
						if (*i == ']'
							&& (i + 1) != end && *(i + 1) == ']'
							&& (i + 2) != end && *(i + 2) == '>'
							)
						{
							text.endCopy(i);
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
					text.beginCopy(i);
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
				auto child = parseImpl(i, end, mode);
				std::cout << "Recursed for " << child->encode() << std::endl;
				tag->children.emplace_back(std::move(child));
#else
				tag->children.emplace_back(parseImpl(i, end, mode));
#endif
				if (i == end)
				{
					text.beginCopy(i);
					break;
				}
				while (string::isSpace(*i) && ++i != end);
				text.beginCopy(i);
				--i; // Cursor is already in the right place but for loop will do `++i`
			}
		}
		text.endCopy(i);
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

	void XmlNode::encodeAndAppendTo(std::string& str, const XmlMode& mode) const SOUP_EXCAL
	{
		if (is_text)
		{
			static_cast<const XmlText*>(this)->encodeAndAppendTo(str);
		}
		else
		{
			static_cast<const XmlTag*>(this)->encodeAndAppendTo(str, mode);
		}
	}

	void XmlNode::encodePrettyAndAppendTo(std::string& str, const XmlMode& mode, unsigned depth) const SOUP_EXCAL
	{
		if (is_text)
		{
			static_cast<const XmlText*>(this)->encodeAndAppendTo(str);
		}
		else
		{
			static_cast<const XmlTag*>(this)->encodePrettyAndAppendTo(str, mode, depth);
		}
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

	void XmlTag::encodeAndAppendTo(std::string& str, const XmlMode& mode) const SOUP_EXCAL
	{
#if SOUP_CPP20
		const bool is_self_closing = mode.self_closing_tags.contains(name);
#else
		const bool is_self_closing = mode.self_closing_tags.count(name);
#endif
		str.push_back('<');
		str.append(name);
		encodeAttributesAndAppendTo(str, mode);
		if (is_self_closing)
		{
			str.append(" /");
		}
		str.push_back('>');
		for (const auto& child : children)
		{
			child->encodeAndAppendTo(str, mode);
		}
		if (!is_self_closing)
		{
			str.append("</");
			str.append(name);
			str.push_back('>');
		}
	}

	void XmlTag::encodePrettyAndAppendTo(std::string& str, const XmlMode& mode, unsigned depth) const SOUP_EXCAL
	{
#if SOUP_CPP20
		const bool is_self_closing = mode.self_closing_tags.contains(name);
#else
		const bool is_self_closing = mode.self_closing_tags.count(name);
#endif
		str.push_back('<');
		str.append(name);
		encodeAttributesAndAppendTo(str, mode);
		if (is_self_closing)
		{
			str.append(" /");
		}
		str.push_back('>');
		const auto child_depth = (depth + 1);
		for (const auto& child : children)
		{
			str.push_back('\n');
			str.append(child_depth * 4, ' ');
			child->encodePrettyAndAppendTo(str, mode, child_depth);
		}
		if (!is_self_closing)
		{
			if (!children.empty())
			{
				str.push_back('\n');
				str.append(depth * 4, ' ');
			}
			str.append("</");
			str.append(name);
			str.push_back('>');
		}
	}

	void XmlTag::encodeAttributesAndAppendTo(std::string& str, const XmlMode& mode) const SOUP_EXCAL
	{
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

	void XmlText::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		std::string contents = this->contents;
		string::replaceAll(contents, "&", "&amp;");
		string::replaceAll(contents, "<", "&lt;");
		string::replaceAll(contents, ">", "&gt;");
		str.append(contents);
	}
}
