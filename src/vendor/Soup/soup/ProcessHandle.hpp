#pragma once

#include "base.hpp"

#include <vector>

#if SOUP_WINDOWS
#include "Module.hpp"
#else
#include "type.hpp"
#endif

#include "Range.hpp"

NAMESPACE_SOUP
{
	class ProcessHandle
#if SOUP_WINDOWS
		: public Module
#endif
	{
#if SOUP_WINDOWS
	public:
		using Module::Module;
#else
	private:
		const pid_t pid;
	public:
		ProcessHandle(const pid_t pid)
			: pid(pid)
		{
		}
#endif

		struct AllocationInfo
		{
			Range range;
			int allowed_access; // memGuard::AllowedAccessFlags
		};
		[[nodiscard]] std::vector<AllocationInfo> getAllocations() const;
	};
}
