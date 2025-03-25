#pragma once

#include <string>
#include <vector>

#include "RegexAlternative.hpp"

NAMESPACE_SOUP
{
	struct RegexGroup
	{
		struct ConstructorState
		{
			mutable const char* it;
			const char* end;
			mutable uint16_t flags;
			mutable size_t next_index = 0;
			mutable std::vector<RegexConstraint**> alternatives_transitions{};

			ConstructorState(const char* it, const char* end, uint16_t flags)
				: it(it), end(end), flags(flags)
			{
			}

			[[nodiscard]] bool hasFlag(uint16_t flag) const noexcept
			{
				return (flags & flag) != 0;
			}
		};

		const size_t index = 0;
		const RegexGroup* parent = nullptr;
		RegexConstraint* initial = nullptr;
		std::vector<RegexAlternative> alternatives{};
		std::string name{};
		bool lookahead_or_lookbehind = false;

		RegexGroup() = default;

		RegexGroup(size_t index)
			: index(index)
		{
		}

		RegexGroup(const char* it, const char* end, uint16_t flags)
			: RegexGroup(ConstructorState(it, end, flags))
		{
		}

		RegexGroup(const ConstructorState& s, bool non_capturing = false);

		RegexGroup(RegexGroup&&) = delete; // Move would invalidate parent pointers

		[[nodiscard]] bool isNonCapturing() const noexcept
		{
			return index == -1;
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

		[[nodiscard]] uint16_t getFlags() const;
		void getFlags(uint16_t& set, uint16_t& unset) const noexcept;

		[[nodiscard]] size_t getCursorAdvancement() const;
	};
}
