#pragma once

#include "fwd.hpp"

#include <vector>

#include "BigBitset.hpp"
#include "Token.hpp"

NAMESPACE_SOUP
{
	class LangDesc
	{
	public:
		BigBitset<0x100 / 8> space_characters{};
		BigBitset<0x100 / 8> token_terminators{};
		BigBitset<0x100 / 8> special_characters{};
		std::vector<std::vector<Token>> token_sets{};

		SOUP_CONSTEXPR20 LangDesc()
		{
			space_characters.enable(' ');
			space_characters.enable('\t');
			space_characters.enable('\n');
			space_characters.enable('\r');
			space_characters.enable(';');

			token_terminators.enable('$');
			token_terminators.enable('(');
			token_terminators.enable(')');
			token_terminators.enable(',');
			token_terminators.enable('.');
			token_terminators.enable(':');
			token_terminators.enable('<');
			token_terminators.enable('[');
			token_terminators.enable('{');
			token_terminators.enable(';');

			special_characters.enable('+');
			special_characters.enable('-');
			special_characters.enable('*');
			special_characters.enable('/');
			special_characters.enable('&');
		}

		Token& addToken(const char* keyword, Token::parse_t parse = nullptr)
		{
			return addToken(keyword, Rgb::WHITE, parse);
		}

		Token& addToken(const char* keyword, Rgb colour, Token::parse_t parse = nullptr)
		{
			return token_sets.emplace_back(std::vector<Token>{ Token{ keyword, colour, parse } }).at(0);
		}

		Token& addTokenWithSamePrecedenceAsPreviousToken(const char* keyword, Token::parse_t parse)
		{
			return addTokenWithSamePrecedenceAsPreviousToken(keyword, Rgb::WHITE, parse);
		}

		Token& addTokenWithSamePrecedenceAsPreviousToken(const char* keyword, Rgb colour, Token::parse_t parse)
		{
			return token_sets.back().emplace_back(Token{ keyword, colour, parse });
		}

		void addBlock(const char* start_keyword, const char* end_keyword);

		[[nodiscard]] std::vector<Lexeme> tokenise(const std::string& code) const;
		static void eraseNlTerminatedComments(std::vector<Lexeme>& ls, const ConstString& prefix);
		static void eraseSpace(std::vector<Lexeme>& ls);
		[[nodiscard]] astBlock parse(std::vector<Lexeme> ls) const;
		[[nodiscard]] astBlock parseImpl(std::vector<Lexeme>& ls) const;
		[[nodiscard]] astBlock parseNoCheck(std::vector<Lexeme>& ls) const;
	private:
		void parseBlock(ParserState& ps, const std::vector<Token>& ts) const;
		void parseBlockRecurse(ParserState& ps, const std::vector<Token>& ts, astBlock* b) const;

	public:
		[[nodiscard]] const Token* findToken(const char* keyword) const;
		[[nodiscard]] const Token* findToken(const std::string& keyword) const;
		[[nodiscard]] const Token* findToken(const Lexeme& l) const;

		[[nodiscard]] FormattedText highlightSyntax(const std::string& code) const;
		[[nodiscard]] FormattedText highlightSyntax(const std::vector<Lexeme>& ls) const;
	};
}
