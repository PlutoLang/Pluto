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

	std::string XmlTag::encode() const noexcept
	{
		std::string str(1, '<');
		str.append(name);
		for (const auto& e : attributes)
		{
			str.push_back(' ');
			//str.push_back('{');
			str.append(e.first);
			//str.push_back('}');
			if (!e.second.empty())
			{
				str.append("=\"");
				str.append(e.second);
				str.push_back('"');
			}
		}
		str.push_back('>');
		for (const auto& child : children)
		{
			if (child->is_text)
			{
				//str.push_back('{');
				std::string contents = static_cast<XmlText*>(child.get())->contents;
				string::replaceAll(contents, "&", "&amp;");
				string::replaceAll(contents, "<", "&lt;");
				string::replaceAll(contents, ">", "&gt;");
				str.append(contents);
				//str.push_back('}');
			}
			else
			{
				str.append(static_cast<XmlTag*>(child.get())->encode());
			}
		}
		str.append("</");
		str.append(name);
		str.push_back('>');
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

	std::vector<UniquePtr<XmlTag>> xml::parse(const std::string& xml)
	{
		std::vector<UniquePtr<XmlTag>> res{};

		auto i = xml.begin();
		do
		{
			res.emplace_back(parse(xml, i));
		} while (i != xml.end() && ++i != xml.end());

		return res;
	}

	UniquePtr<XmlTag> xml::parseAndDiscardMetadata(const std::string& xml)
	{
		auto tags = parse(xml);

		for (auto i = tags.begin(); i != tags.end(); )
		{
			if ((*i)->name.c_str()[0] == '?'
				|| (*i)->name.c_str()[0] == '!'
				)
			{
				i = tags.erase(i);
			}
			else
			{
				++i;
			}
		}

		if (tags.size() == 1)
		{
			SOUP_MOVE_RETURN(tags.at(0));
		}

		auto body = soup::make_unique<XmlTag>();
		body->name = "body";
		body->children.reserve(tags.size());
		for (auto& tag : tags)
		{
			body->children.emplace_back(std::move(tag));
		}
		return body;
	}

	UniquePtr<XmlTag> xml::parse(const std::string& xml, std::string::const_iterator& i)
	{
		auto tag = soup::make_unique<XmlTag>();
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
							tag->attributes.emplace_back(std::move(name), std::string());
							name.clear();
						}
						break;
					}
					if (*i == ' ')
					{
						name.endCopy(xml, i);
						if (!name.empty())
						{
							tag->attributes.emplace_back(std::move(name), std::string());
							name.clear();
						}
						name.beginCopy(xml, i + 1);
					}
					else if (*i == '/')
					{
						name.endCopy(xml, i);
						if (!name.empty())
						{
							tag->attributes.emplace_back(std::move(name), std::string());
							name.clear();
						}
						if ((i + 1) != xml.end()
							&& *(i + 1) == '>'
							)
						{
#if DEBUG_PARSE
							std::cout << tag->name << " taking the easy way out" << std::endl;
#endif
							++i;
							return tag;
						}
						name.beginCopy(xml, i + 1);
					}
					else if (*i == '=')
					{
						name.endCopy(xml, i);
						StringBuilder value;
						++i;
						if (*i == '"')
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
						else
						{
#if DEBUG_PARSE
							std::cout << "Attribute " << name << " has no value";
#endif
							tag->attributes.emplace_back(std::move(name), std::string());
						}
						name.clear();
						name.beginCopy(xml, i + 1);
					}
				}
			}
			if (tag->name.c_str()[0] == '?' // <?xml ... ?>
				|| tag->name.c_str()[0] == '!' // <!DOCTYPE ...>
				)
			{
				return tag; // Won't have children
			}
			++i;
		}
		StringBuilder text;
		text.beginCopy(xml, i);
		for (;; ++i)
		{
			if (i == xml.end())
			{
				break;
			}
			if (*i == '<')
			{
				text.endCopy(xml, i);
				if (!text.empty())
				{
#if DEBUG_PARSE
					std::cout << "Copied text: " << text << std::endl;
#endif
					tag->children.emplace_back(soup::make_unique<XmlText>(std::move(text)));
					text.clear();
				}

				if ((i + 1) != xml.end()
					&& *(i + 1) == '/'
					)
				{
					i += 2;
					StringBuilder tbc_tag;
					tbc_tag.beginCopy(xml, i);
					for (;; ++i)
					{
						if (i == xml.end())
						{
							break;
						}
						if (*i == '>')
						{
							break;
						}
					}
					tbc_tag.endCopy(xml, i);
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
#if DEBUG_PARSE
				auto child = parse(xml, i);
				std::cout << "Recursed for " << child->name << ": " << child->encode() << std::endl;
				tag->children.emplace_back(std::move(child));
#else
				tag->children.emplace_back(parse(xml, i));
#endif
				if (i == xml.end())
				{
					break;
				}
				text.beginCopy(xml, i + 1);
			}
		}
		text.endCopy(xml, i);
		if (!text.empty())
		{
#if DEBUG_PARSE
			std::cout << "Copied text before return: " << text << std::endl;
#endif
			tag->children.emplace_back(soup::make_unique<XmlText>(std::move(text)));
			text.clear();
		}
		return tag;
	}
}
