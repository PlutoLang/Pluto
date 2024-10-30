#include "rflParser.hpp"

#include "LangDesc.hpp"
#include "rflFunc.hpp"
#include "rflStruct.hpp"
#include "rflType.hpp"
#include "rflVar.hpp"

NAMESPACE_SOUP
{
	rflParser::rflParser(const std::string& code)
		: LexemeParser(LangDesc{}, code)
	{
	}

	rflType rflParser::readType()
	{
		rflType type;
		type.name = readRawType();
		type.at = readAccessType();
		return type;
	}

	std::string rflParser::readRawType()
	{
		std::string str;
		str = readLiteral();
		if (str == "const")
		{
			str.push_back(' ');
			str.append(readLiteral());
		}
		if (str == "unsigned")
		{
			str.push_back(' ');
			str.append(readLiteral());
		}
		if ((i - 1)->val.getString() == "long" && peekLiteral() == "long")
		{
			str.push_back(' ');
			str.append(readLiteral());
		}
		return str;
	}

	rflType::AccessType rflParser::readAccessType()
	{
		const auto next_literal = peekLiteral();
		if (next_literal == "*")
		{
			advance();
			return rflType::POINTER;
		}
		else if (next_literal == "&")
		{
			advance();
			return rflType::REFERENCE;
		}
		else if (next_literal == "&&")
		{
			advance();
			return rflType::RVALUE_REFERENCE;
		}
		return rflType::DIRECT;
	}

	rflVar rflParser::readVar()
	{
		rflVar var;
		readVar(var);
		return var;
	}

	void rflParser::readVar(rflVar& var)
	{
		var.type = readType();
		var.name = readLiteral();
	}

	rflFunc rflParser::readFunc()
	{
		rflFunc f;
		f.return_type = readType();
		f.name = readLiteral();
		SOUP_ASSERT(readLiteral() == "(");
		if (peekLiteral() != ")")
		{
			while (true)
			{
				readVar(f.parameters.emplace_back(rflVar{}));
				if (peekLiteral() == ",") // More parameters?
				{
					advance();
				}
				else if (readLiteral() == ")") // End of parameters?
				{
					break;
				}
				else // Wat?
				{
					SOUP_ASSERT_UNREACHABLE;
				}
			}
		}
		else
		{
			advance();
		}
		if (hasMore())
		{
			align();
		}
		return f;
	}

	rflStruct rflParser::readStruct()
	{
		rflMember::Accessibility accessibility = rflMember::PUBLIC; // struct & union
		auto st = readLiteral();
		if (st == "class")
		{
			accessibility = rflMember::PRIVATE;
		}
		else if (st != "struct")
		{
			SOUP_ASSERT_UNREACHABLE;
		}
		rflStruct desc;
		desc.name = readLiteral();
		if (desc.name == "{")
		{
			desc.name.clear();
		}
		else
		{
			SOUP_ASSERT(readLiteral() == "{");
		}
		while (peekLiteral() != "}")
		{
			const auto type = readRawType();
			do
			{
				rflMember& member = desc.members.emplace_back(rflMember{});
				member.type.name = type;
				member.type.at = readAccessType();
				member.name = readLiteral();
				member.accessibility = accessibility;
			} while (peekLiteral() == "," && (advance(), true));
		}
		if (hasMore())
		{
			advance(); // skip '}'
			align();
		}
		return desc;
	}

	void rflParser::align()
	{
		while (hasMore()
			&& (
				i->isSpace()
				|| (
					i->isLiteral()
					&& (i->getLiteral() == "//"
						|| i->getLiteral().at(0) == '#'
						)
					)
				)
			)
		{
			if (!i->isSpace())
			{
				do
				{
					advance();
				} while (hasMore() && !i->isNewLine());
			}
			if (!hasMore())
			{
				break;
			}
			advance();
		}
	}

	std::string rflParser::readLiteral()
	{
		align();
		SOUP_ASSERT(hasMore());
		SOUP_ASSERT(i->isLiteral());
		return (i++)->val.getString();
	}

	std::string rflParser::peekLiteral()
	{
		align();
		if (hasMore() && i->isLiteral())
		{
			return i->val.getString();
		}
		return {};
	}
}
