#pragma once

#include <optional>
#include <string>
#include <vector>

#include "fwd.hpp"
#include "Regex.hpp"
#include "RegexMatchResult.hpp"

NAMESPACE_SOUP
{
	struct RegexMatcher
	{
		struct RollbackPoint
		{
			const RegexConstraint* c;
			const char* it;
			RegexMatchResult result{};
		};

		const RegexConstraint* c;
		const char* it;
		const char* const begin;
		const char* const end;
		std::vector<RollbackPoint> rollback_points{};
		std::vector<const char*> checkpoints{};
		RegexMatchResult result{};

		RegexMatcher(const Regex& r, const char* begin, const char* end)
			: c(r.group.initial), begin(begin), end(end)
		{
		}

		void reset(const Regex& r) noexcept
		{
			c = r.group.initial;
			rollback_points.clear();
			checkpoints.clear();
			result.groups.clear();
		}

		void saveRollback(const RegexConstraint* rollback_transition)
		{
			rollback_points.emplace_back(RollbackPoint{ rollback_transition, it, result });
		}

		void restoreRollback()
		{
			c = rollback_points.back().c;
			it = rollback_points.back().it;
			result = std::move(rollback_points.back().result);
			rollback_points.pop_back();
		}

		bool shouldSaveCheckpoint() noexcept
		{
			if (reinterpret_cast<uintptr_t>(c) & 0b1)
			{
				c = reinterpret_cast<const RegexConstraint*>(reinterpret_cast<uintptr_t>(c) & ~0b1);
				SOUP_ASSERT(c != nullptr);
				return true;
			}
			return false;
		}

		bool shouldResetCapture() noexcept
		{
			if (reinterpret_cast<uintptr_t>(c) & 0b10)
			{
				c = reinterpret_cast<const RegexConstraint*>(reinterpret_cast<uintptr_t>(c) & ~0b10);
				return true;
			}
			return false;
		}

		void saveCheckpoint()
		{
			checkpoints.emplace_back(it);
		}

		void restoreCheckpoint()
		{
			it = checkpoints.back();
			checkpoints.pop_back();
		}

		void insertMissingCapturingGroups(const RegexGroup* g)
		{
			for (; g; g = g->parent)
			{
				if (g->lookahead_or_lookbehind)
				{
					break;
				}
				if (g->isNonCapturing())
				{
					continue;
				}
				while (g->index >= this->result.groups.size())
				{
					this->result.groups.emplace_back(std::nullopt);
				}
				if (!this->result.groups.at(g->index).has_value())
				{
					this->result.groups.at(g->index) = RegexMatchedGroup{ g->name, this->it, this->it };
				}
			}
		}
	};
}
