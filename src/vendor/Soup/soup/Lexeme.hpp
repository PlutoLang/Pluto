#pragma once

#include "Mixed.hpp"

NAMESPACE_SOUP
{
	struct Lexeme
	{
		inline static const char* VAL = "value";
		inline static const char* LITERAL = "literal";
		inline static const char* SPACE = "space";

		const char* token_keyword;
		Mixed val;
		char string_type = 0;

		[[nodiscard]] bool isBuiltin() const noexcept
		{
			return token_keyword == VAL
				|| token_keyword == LITERAL
				|| token_keyword == SPACE
				;
		}

		[[nodiscard]] bool isLiteral() const noexcept
		{
			return token_keyword == LITERAL;
		}

		[[nodiscard]] bool isSpace() const noexcept
		{
			return token_keyword == SPACE;
		}

		[[nodiscard]] std::string getSourceString() const noexcept
		{
			if (isBuiltin())
			{
				if (token_keyword == VAL
					&& val.isString()
					)
				{
					std::string s(1, string_type);
					s.append(val.getString());
					s.push_back(string_type);
					return s;
				}
				return val.toString();
			}
			return token_keyword;
		}

		[[nodiscard]] std::string toString(const std::string& prefix = {}) const noexcept
		{
			std::string str = token_keyword;
			if (auto val_str = val.toString(prefix); !val_str.empty())
			{
				str.append(": ");
				str.append(val_str);
			}
			else if (token_keyword == VAL)
			{
				str.append(": ");
				str.append(val.getTypeName());
			}
			return str;
		}

		[[nodiscard]] bool isNewLine() const noexcept
		{
			return token_keyword == SPACE
				&& val.getString() == "\n"
				;
		}

		[[nodiscard]] bool isLiteral(const std::string& b) const noexcept
		{
			return isLiteral()
				&& getLiteral() == b
				;
		}

		[[nodiscard]] std::string getLiteral() const
		{
			return val.getString();
		}
	};
}
