#include "Process.hpp"

#if SOUP_WINDOWS
	#undef UNICODE
	#include <tlhelp32.h>

	#include "HandleRaii.hpp"
	#include "VirtualHandleRaii.hpp"
#else
	#include "FileReader.hpp"
	#include "Range.hpp"
	#include "Regex.hpp"
	#include "string.hpp"
#endif
#include "os.hpp"

NAMESPACE_SOUP
{
	UniquePtr<Process> Process::current()
	{
		return get(os::getProcessId());
	}

	UniquePtr<Process> Process::get(pid_t id)
	{
#if SOUP_WINDOWS
		HandleRaii hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap)
		{
			PROCESSENTRY32 entry;
			entry.dwSize = sizeof(entry);
			if (Process32First(hSnap, &entry))
			{
				do
				{
					if (entry.th32ProcessID == id)
					{
						return soup::make_unique<Process>(entry.th32ProcessID, entry.szExeFile);
					}
				} while (Process32Next(hSnap, &entry));
			}
		}
		return {};
#else
		return soup::make_unique<Process>(id);
#endif
	}

#if SOUP_WINDOWS
	UniquePtr<Process> Process::get(const char* name)
	{
		HandleRaii hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap)
		{
			PROCESSENTRY32 entry;
			entry.dwSize = sizeof(entry);
			if (Process32First(hSnap, &entry))
			{
				do
				{
					if (strcmp(entry.szExeFile, name) == 0)
					{
						return soup::make_unique<Process>(entry.th32ProcessID, entry.szExeFile);
					}
				} while (Process32Next(hSnap, &entry));
			}
		}
		return {};
	}

	std::vector<UniquePtr<Process>> Process::getAll()
	{
		std::vector<UniquePtr<Process>> res{};
		HandleRaii hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap)
		{
			PROCESSENTRY32 entry;
			entry.dwSize = sizeof(entry);
			if (Process32First(hSnap, &entry))
			{
				do
				{
					res.emplace_back(soup::make_unique<Process>(entry.th32ProcessID, entry.szExeFile));
				} while (Process32Next(hSnap, &entry));
			}
		}
		return res;
	}
#endif

	std::shared_ptr<ProcessHandle> Process::open() const
	{
#if SOUP_WINDOWS
		const DWORD desired_access = PROCESS_CREATE_THREAD | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE;

		HandleRaii hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, id);
		if (hSnap)
		{
			MODULEENTRY32 entry;
			entry.dwSize = sizeof(entry);
			if (Module32First(hSnap, &entry))
			{
				do
				{
					if (this->id == entry.th32ProcessID)
					{
						return std::make_shared<ProcessHandle>(make_unique<VirtualHandleRaii>(OpenProcess(desired_access, FALSE, id)), Range(entry.modBaseAddr, entry.modBaseSize));
					}
				} while (Module32Next(hSnap, &entry));
			}
		}
		return {};
#else
		return std::make_shared<ProcessHandle>(this->id);
#endif
	}
}
