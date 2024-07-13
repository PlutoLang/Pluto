#pragma once

#include <string>
#include <vector>

#include "fwd.hpp"
#include "Lexeme.hpp"

NAMESPACE_SOUP
{
	struct LexemeParser
	{
		std::vector<Lexeme> tks;
		std::vector<Lexeme>::iterator i;

		explicit LexemeParser(const LangDesc& ld, const std::string& code);
		explicit LexemeParser(std::vector<Lexeme>&& tks);

		[[nodiscard]] bool hasMore() const noexcept
		{
			return tks.end() != i;
		}

		[[nodiscard]] const char* getTokenKeyword() const noexcept
		{
			if (hasMore())
			{
				return i->token_keyword;
			}
			return nullptr;
		}

		void advance() noexcept
		{
			++i;
		}
	};
}
