#pragma once

#include <optional>
#include <vector>

#include "fwd.hpp"

#include "Pointer.hpp"

NAMESPACE_SOUP
{
	class Range
	{
	public:
		Pointer base;
		size_t size;

		Range() noexcept = default;

		Range(Pointer base, size_t size) noexcept
			: base(base), size(size)
		{
		}

		[[nodiscard]] Pointer end() const noexcept
		{
			return base.add(size);
		}

		[[nodiscard]] bool contains(Pointer h) const noexcept
		{
			return h.as<uintptr_t>() >= base.as<uintptr_t>() && h.as<uintptr_t>() <= end().as<uintptr_t>();
		}

		[[nodiscard]] static bool pattern_matches(uint8_t* target, const std::optional<uint8_t>* sig, size_t length) noexcept;

		[[nodiscard]] Pointer scan(const Pattern& sig) const noexcept
		{
			Pointer ptr{};
			SOUP_UNUSED(scanWithMultipleResults(sig, &ptr, 1));
			return ptr;
		}

		template <size_t S>
		[[nodiscard]] size_t scanWithMultipleResults(const Pattern& sig, Pointer(&buf)[S]) const noexcept
		{
			return scanWithMultipleResults(sig, buf, S);
		}

		[[nodiscard]] size_t scanWithMultipleResults(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept;
	protected:
		[[nodiscard]] size_t scanWithMultipleResultsSimd(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept;
#if SOUP_X86 && SOUP_BITS == 64
		[[nodiscard]] size_t scanWithMultipleResultsAvx2(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept;
		[[nodiscard]] size_t scanWithMultipleResultsAvx512(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept;
#endif
	};
}
