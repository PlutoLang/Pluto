#pragma once

#include "base.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#endif

NAMESPACE_SOUP
{
	enum memProtFlags : int
	{
		MEM_PROT_READ = 0x01, // equals PROT_READ on Linux
		MEM_PROT_WRITE = 0x02, // equals PROT_WRITE on Linux
		MEM_PROT_EXEC = 0x04, // equals PROT_EXEC on Linux
	};

#if SOUP_WINDOWS
	[[nodiscard]] constexpr DWORD memProtFlagsToProtect(int prot_flags)
	{
		DWORD protect = 0x01; // PAGE_NOACCESS
		if (prot_flags & MEM_PROT_WRITE)
		{
			protect = 0x04; // PAGE_READWRITE
		}
		else if (prot_flags & MEM_PROT_READ)
		{
			protect = 0x02; // PAGE_READONLY
		}
		if (prot_flags & MEM_PROT_EXEC)
		{
			protect <<= 4;
			// PAGE_NOACCESS (0x01) -> PAGE_EXECUTE (0x10)
			// PAGE_READONLY (0x02) -> PAGE_EXECUTE_READ (0x20)
			// PAGE_READWRITE (0x04) -> PAGE_EXECUTE_READWRITE (0x40)
		}
		return protect;
	}
#endif

}
