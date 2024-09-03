#include "filesystem.hpp"

#include <fstream>

#if SOUP_WINDOWS
#include <windows.h>
#include <shlobj.h> // CSIDL_COMMON_APPDATA

#pragma comment(lib, "Shell32.lib") // SHGetFolderPathW
#pragma comment(lib, "User32.lib") // SendInput
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "rand.hpp"
#if SOUP_CPP20
#include "string.hpp"
#endif

NAMESPACE_SOUP
{
	std::filesystem::path filesystem::u8path(const std::string& str)
	{
#if SOUP_CPP20
		return soup::string::toUtf8Type(str);
#else
		return std::filesystem::u8path(str);
#endif
	}

	bool filesystem::exists_case_sensitive(const std::filesystem::path& p)
	{
		return std::filesystem::exists(p)
#if SOUP_WINDOWS
			&& p.filename() == std::filesystem::canonical(p).filename()
#endif
			;
	}

	intptr_t filesystem::filesize(const std::filesystem::path& path)
	{
		// This is not guaranteed to work, but works on UNIX, and on Windows in binary mode.
		std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
		return static_cast<intptr_t>(in.tellg());
	}

	bool filesystem::replace(const std::filesystem::path& replaced, const std::filesystem::path& replacement)
	{
#if SOUP_WINDOWS
		SOUP_IF_UNLIKELY (!std::filesystem::exists(replaced))
		{
			return MoveFileW(replacement.c_str(), replaced.c_str()) != 0;
		}
		return ReplaceFileW(replaced.c_str(), replacement.c_str(), nullptr, 0, 0, 0) != 0;
#else
		return ::rename(replacement.c_str(), replaced.c_str()) == 0;
#endif
	}

	std::filesystem::path filesystem::tempfile(const std::string& ext)
	{
		std::filesystem::path path;
		do
		{
			auto file = rand.str<std::string>(20);
			if (!ext.empty())
			{
				if (ext.at(0) != '.')
				{
					file.push_back('.');
				}
				file.append(ext);
			}
			path = std::filesystem::temp_directory_path();
			path /= file;
		} while (std::filesystem::exists(path));
		return path;
	}

	std::filesystem::path filesystem::getProgramData() SOUP_EXCAL
	{
#if SOUP_WINDOWS
		wchar_t szPath[MAX_PATH];
		if (SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath) == 0)
		{
			return szPath;
		}
		return "C:\\ProgramData";
#else
		return "/var/lib";
#endif
	}

#if SOUP_WINDOWS
	static char empty_file_data = 0;
#endif

	void* filesystem::createFileMapping(const std::filesystem::path& path, size_t& out_len)
	{
		void* addr = nullptr;
#if SOUP_WINDOWS
		HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		SOUP_IF_LIKELY (f != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER liSize;
			SOUP_IF_LIKELY (GetFileSizeEx(f, &liSize))
			{
				out_len = static_cast<size_t>(liSize.QuadPart);
				if (out_len == 0)
				{
					addr = &empty_file_data;
				}
				else
				{
					HANDLE m = CreateFileMappingA(f, nullptr, PAGE_READONLY, liSize.HighPart, liSize.LowPart, NULL);
					SOUP_IF_LIKELY(m != NULL)
					{
						addr = MapViewOfFile(m, FILE_MAP_READ, 0, 0, out_len);
						CloseHandle(m);
					}
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

	void filesystem::destroyFileMapping(void* addr, size_t len)
	{
#if SOUP_WINDOWS
		if (addr != &empty_file_data)
		{
			UnmapViewOfFile(addr);
		}
#else
		munmap(addr, len);
#endif
	}
}
