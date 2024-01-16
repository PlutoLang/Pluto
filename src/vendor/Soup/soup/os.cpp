#include "os.hpp"

#include <array>
#include <cstdio>
#include <cstring> // memcpy
#include <fstream>

#if SOUP_WINDOWS
#include <Psapi.h>
#include <ShlObj.h> // CSIDL_COMMON_APPDATA

#pragma comment(lib, "Shell32.lib") // SHGetFolderPathW
#pragma comment(lib, "User32.lib") // SendInput

#include "Exception.hpp"
#include "ObfusString.hpp"
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "rand.hpp"
#include "string.hpp"
#include "UniquePtr.hpp"

namespace soup
{
	void* os::createFileMapping(std::filesystem::path path, size_t& out_len)
	{
		void* addr = nullptr;
#if SOUP_WINDOWS
		HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		SOUP_IF_LIKELY (f != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER liSize;
			SOUP_IF_LIKELY (GetFileSizeEx(f, &liSize))
			{
				out_len = liSize.QuadPart;
				HANDLE m = CreateFileMappingA(f, nullptr, PAGE_READONLY, liSize.HighPart, liSize.LowPart, NULL);
				SOUP_IF_LIKELY (m != NULL)
				{
					addr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, liSize.QuadPart);
					CloseHandle(m);
				}
			}
			CloseHandle(f);
		}
#else
		int f = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
		SOUP_IF_LIKELY (f != -1)
		{
			struct stat st;
			SOUP_IF_LIKELY (fstat(f, &st) != -1)
			{
				out_len = st.st_size;
				addr = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, f, 0);
			}
			::close(f);
		}
#endif
		return addr;
	}

	void os::destroyFileMapping(void* addr, size_t len)
	{
#if SOUP_WINDOWS
		UnmapViewOfFile(addr);
#else
		munmap(addr, len);
#endif
	}
}
