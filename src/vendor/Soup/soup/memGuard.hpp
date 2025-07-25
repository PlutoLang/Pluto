#pragma once

#include <cstddef> // size_t

#include "base.hpp"

#if SOUP_WINDOWS
	#include <windows.h>
#endif

NAMESPACE_SOUP
{
	struct memGuard
	{
		enum AllowedAccessFlags : int
		{
			ACC_READ = 0x01,
			ACC_WRITE = 0x02,
			ACC_EXEC = 0x04,

			ACC_RWX = ACC_READ | ACC_WRITE | ACC_EXEC
		};
#if SOUP_WINDOWS
		[[nodiscard]] static DWORD allowedAccessToProtect(int allowed_access);
		[[nodiscard]] static int protectToAllowedAccess(DWORD protect);
#endif

		[[nodiscard]] static void* alloc(size_t len, int allowed_access);
		static void free(void* addr, size_t len);
		static void setAllowedAccess(void* addr, size_t len, int allowed_access, int* old_allowed_access = nullptr);
		static int getAllowedAccess(void* addr);
	};
}
