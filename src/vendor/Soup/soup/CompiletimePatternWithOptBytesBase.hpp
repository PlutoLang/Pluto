#pragma once

#include "CompiletimePatternBase.hpp"

#include <optional>
#include <vector>

NAMESPACE_SOUP
{
	struct CompiletimePatternWithOptBytesBase : public CompiletimePatternBase
	{
		virtual std::vector<std::optional<uint8_t>> getVec() const = 0;
	};
}
