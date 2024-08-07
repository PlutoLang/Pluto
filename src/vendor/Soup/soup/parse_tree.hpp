#pragma once

#include "fwd.hpp"

#include <string>

#include "Lexeme.hpp"
#include "Op.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct astNode
	{
		enum Type : uint8_t
		{
			BLOCK = 0,
			LEXEME,
			OP,
		};

		const Type type;

		astNode(Type type) noexcept
			: type(type)
		{
		}

		virtual ~astNode() = default;

		[[nodiscard]] bool isValue() const noexcept;

		[[nodiscard]] std::string toString(const std::string& prefix = {}) const;

		void compile(Writer& w) const;
	};

	struct astBlock : public astNode
	{
		std::vector<UniquePtr<astNode>> children{};
		std::vector<UniquePtr<astNode>> param_literals;

		astBlock(std::vector<UniquePtr<astNode>>&& children = {})
			: astNode(BLOCK), children(std::move(children))
		{
		}

		void checkUnexpected() const;

		[[nodiscard]] std::string toString(std::string prefix = {}) const;

		void compile(Writer& w) const;
	};

	struct LexemeNode : public astNode
	{
		Lexeme lexeme;

		LexemeNode(Lexeme lexeme)
			: astNode(LEXEME), lexeme(std::move(lexeme))
		{
		}

		[[nodiscard]] std::string toString(const std::string& prefix = {}) const;

		void compile(Writer& w) const;
	};

	struct OpNode : public astNode
	{
		Op op;

		OpNode(Op&& op)
			: astNode(OP), op(std::move(op))
		{
		}

		[[nodiscard]] std::string toString(std::string prefix = {}) const;

		void compile(Writer& w) const;
	};
}
