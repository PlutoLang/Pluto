#pragma once

#include "LexemeParser.hpp"

#include "fwd.hpp"
#include "rflType.hpp"

NAMESPACE_SOUP
{
	struct rflParser : public LexemeParser
	{
		explicit rflParser(const std::string& code);

		[[nodiscard]] rflType readType();
		[[nodiscard]] std::string readRawType();
		[[nodiscard]] rflType::AccessType readAccessType();
		[[nodiscard]] rflVar readVar();
		void readVar(rflVar& var);
		[[nodiscard]] rflFunc readFunc();
		[[nodiscard]] rflStruct readStruct();

		void align();
		[[nodiscard]] std::string readLiteral();
		[[nodiscard]] std::string peekLiteral();
	};
}
