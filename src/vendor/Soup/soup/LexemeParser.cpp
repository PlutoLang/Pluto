#include "LexemeParser.hpp"

#include "LangDesc.hpp"

NAMESPACE_SOUP
{
	LexemeParser::LexemeParser(const LangDesc& ld, const std::string& code)
		: LexemeParser(ld.tokenise(code))
	{
	}

	LexemeParser::LexemeParser(std::vector<Lexeme>&& tks)
		: tks(std::move(tks)), i(this->tks.begin())
	{
	}
}
