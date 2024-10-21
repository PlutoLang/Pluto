#include "ParserState.hpp"

#include <string>

#include "LangDesc.hpp"
#include "Lexeme.hpp"
#include "ParseError.hpp"
#include "parse_tree.hpp"
#include "UniquePtr.hpp"

#define ENSURE_SANITY false

NAMESPACE_SOUP
{
	void ParserState::setOp(int type)
	{
		op.type = type;
	}

	void ParserState::consumeLefthandValue()
	{
		auto node = popLefthand();
		if (!node->isValue())
		{
			std::string err = static_cast<LexemeNode*>(i->get())->lexeme.token_keyword;
			err.append(" expected lefthand value, found ");
			err.append(node->toString());
			SOUP_THROW(ParseError(std::move(err)));
		}
		pushArgNode(std::move(node));
#if ENSURE_SANITY
		ensureValidIterator();
#endif
	}

	void ParserState::consumeRighthandValue()
	{
		auto node = popRighthand();
		if (!node->isValue())
		{
			throwExpectedRighthandValue(node);
		}
		pushArgNode(std::move(node));
#if ENSURE_SANITY
		ensureValidIterator();
#endif
	}

	void ParserState::throwExpectedRighthandValue(const UniquePtr<astNode>& node)
	{
		std::string err = static_cast<LexemeNode*>(i->get())->lexeme.token_keyword;
		err.append(" expected righthand value, found ");
		err.append(node->toString());
		SOUP_THROW(ParseError(std::move(err)));
	}

	void ParserState::pushArg(Mixed&& val)
	{
		pushArgNode(soup::make_unique<LexemeNode>(Lexeme{ Lexeme::VAL, std::move(val) }));
	}

	void ParserState::pushArgNode(UniquePtr<astNode>&& node)
	{
		op.args.emplace_back(std::move(node));
	}

	void ParserState::setArgs(std::vector<UniquePtr<astNode>>&& args)
	{
		op.args = std::move(args);
	}

	void ParserState::pushLefthand(Mixed&& val)
	{
		pushLefthand(Lexeme{ Lexeme::VAL, std::move(val) });
	}

	void ParserState::pushLefthand(Lexeme&& l)
	{
		pushLefthandNode(soup::make_unique<LexemeNode>(std::move(l)));
	}

	void ParserState::pushLefthandNode(UniquePtr<astNode>&& node)
	{
		i = b->children.insert(i, std::move(node));
		++i;
	}

	astNode* ParserState::peekLefthand() const noexcept
	{
		if (i != b->children.begin())
		{
			return (i - 1)->get();
		}
		return nullptr;
	}

	UniquePtr<astNode> ParserState::popLefthand()
	{
		if (i == b->children.begin())
		{
			std::string err = static_cast<LexemeNode*>(i->get())->lexeme.token_keyword;
			err.append(" expected lefthand, found start of block");
			SOUP_THROW(ParseError(std::move(err)));
		}
		--i;
		UniquePtr<astNode> ret = std::move(*i);
		i = b->children.erase(i);
#if ENSURE_SANITY
		ensureValidIterator();
#endif
		return ret;
	}

	astNode* ParserState::peekRighthand() const
	{
		checkRighthand();
		return (i + 1)->get();
	}

	UniquePtr<astNode> ParserState::popRighthand()
	{
		checkRighthand();
		++i;
		UniquePtr<astNode> ret = std::move(*i);
		i = b->children.erase(i);
		--i;
#if ENSURE_SANITY
		ensureValidIterator();
#endif
		return ret;
	}

	UniquePtr<astBlock> ParserState::collapseRighthandBlock(const char* end_token)
	{
		auto b = soup::make_unique<astBlock>();
		for (UniquePtr<astNode> node;
			node = popRighthand(), node->type != astNode::LEXEME || static_cast<LexemeNode*>(node.get())->lexeme.token_keyword != end_token;
			)
		{
			b->children.emplace_back(std::move(node));
		}
		return b;
	}

	const Token& ParserState::getToken() const
	{
		return *ld->findToken(static_cast<LexemeNode*>(i->get())->lexeme);
	}

	void ParserState::checkRighthand() const
	{
		if ((i + 1) == b->children.end())
		{
			std::string err = static_cast<LexemeNode*>(i->get())->lexeme.token_keyword;
			err.append(" expected righthand, found end of block");
			SOUP_THROW(ParseError(std::move(err)));
		}
	}

	void ParserState::ensureValidIterator() const
	{
		SOUP_ASSERT((*i)->type == astNode::LEXEME
			&& static_cast<LexemeNode*>(i->get())->lexeme.token_keyword != Lexeme::VAL
		);
	}
}
