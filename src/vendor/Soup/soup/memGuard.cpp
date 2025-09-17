#include "memGuard.hpp"

#if SOUP_WINDOWS
	#include <windows.h>
#else
	#include <sys/mman.h>
#endif

#include "Process.hpp"
#include "ProcessHandle.hpp"

NAMESPACE_SOUP
{
#if SOUP_WINDOWS
	DWORD memGuard::allowedAccessToProtect(int allowed_access) noexcept
	{
		DWORD protect = 0x01; // PAGE_NOACCESS
		if (allowed_access & ACC_WRITE)
		{
			protect = 0x04; // PAGE_READWRITE
		}
		else if (allowed_access & ACC_READ)
		{
			protect = 0x02; // PAGE_READONLY
		}
		if (allowed_access & ACC_EXEC)
		{
			protect <<= 4;
			// PAGE_NOACCESS (0x01) -> PAGE_EXECUTE (0x10)
			// PAGE_READONLY (0x02) -> PAGE_EXECUTE_READ (0x20)
			// PAGE_READWRITE (0x04) -> PAGE_EXECUTE_READWRITE (0x40)
		}
		return protect;
	}

	int memGuard::protectToAllowedAccess(DWORD protect) noexcept
	{
		switch (protect & ~PAGE_GUARD)
		{
		case PAGE_READONLY: return ACC_READ;
		case PAGE_READWRITE: case PAGE_WRITECOPY: return ACC_READ | ACC_WRITE;
		case PAGE_EXECUTE: return ACC_EXEC;
		case PAGE_EXECUTE_READ: return ACC_EXEC | ACC_READ;
		case PAGE_EXECUTE_READWRITE: case PAGE_EXECUTE_WRITECOPY: return ACC_EXEC | ACC_READ | ACC_WRITE;
		}
		return 0;
	}
#else
	static_assert(PROT_READ == memGuard::ACC_READ);
	static_assert(PROT_WRITE == memGuard::ACC_WRITE);
	static_assert(PROT_EXEC == memGuard::ACC_EXEC);
#endif

	void* memGuard::alloc(size_t len, int allowed_access) noexcept
	{
#if SOUP_WINDOWS
		return VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, allowedAccessToProtect(allowed_access));
#else
		int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	#if SOUP_APPLE
		if ((allowed_access & (memGuard::ACC_WRITE | memGuard::ACC_EXEC)) == (memGuard::ACC_WRITE | memGuard::ACC_EXEC))
		{
			flags |= MAP_JIT;
		}
	#endif
		if (const auto block = mmap(nullptr, len, allowed_access, flags, -1, 0); block != MAP_FAILED)
		{
			return block;
		}
		return nullptr;
#endif
	}

	void memGuard::free(void* addr, size_t len) noexcept
	{
#if SOUP_WINDOWS
		VirtualFree(addr, len, MEM_DECOMMIT);
#else
		munmap(addr, len);
#endif
	}

	void memGuard::setAllowedAccess(void* addr, size_t len, int allowed_access, int* old_allowed_access) noexcept
	{
#if SOUP_WINDOWS
		DWORD oldprotect;
		VirtualProtect(addr, len, allowedAccessToProtect(allowed_access), &oldprotect);
		if (old_allowed_access)
		{
			*old_allowed_access = protectToAllowedAccess(oldprotect);
		}
#else
		if (old_allowed_access)
		{
			*old_allowed_access = getAllowedAccess(addr);
		}
		mprotect(addr, len, allowed_access);
#endif
	}

	int memGuard::getAllowedAccess(void* addr)
	{
		for (const auto& a : Process::current()->open()->getAllocations())
		{
			if (a.range.contains(addr))
			{
				return a.allowed_access;
			}
		}
		return 0;
	}
}
