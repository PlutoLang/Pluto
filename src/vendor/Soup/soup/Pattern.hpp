#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "fwd.hpp"
#include "base.hpp"

NAMESPACE_SOUP
{
	class Pattern
	{
	public:
		std::vector<std::optional<uint8_t>> bytes;

		explicit Pattern() = default;
		Pattern(const CompiletimePatternWithOptBytesBase& sig);
		Pattern(const std::string& str);
		Pattern(const char* str, size_t len);
		Pattern(const char* bin, const char* mask);

		[[nodiscard]] bool matches(uint8_t* target) const noexcept; // target must have a size of at least `bytes.size()`

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

#if SOUP_X86 && SOUP_BITS == 64
		[[nodiscard]] size_t getMostUniqueByteIndex() const noexcept;
#endif

		[[nodiscard]] bool hasWildcards() const noexcept;

		bool io(Writer& w) noexcept;
		bool io(Reader& r) SOUP_EXCAL;
	};
}
