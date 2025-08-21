#include "Process.hpp"

#if SOUP_WINDOWS
	#undef UNICODE
	#include <tlhelp32.h>

	#include "HandleRaii.hpp"
	#include "VirtualHandleRaii.hpp"
#else
	#include <filesystem>

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
#else
		std::ifstream commFile("/proc/" + std::to_string(id) + "/comm");
		if (commFile.is_open())
		{
			std::string procName;
			std::getline(commFile, procName);
			commFile.close();
			return soup::make_unique<Process>(id, std::move(procName));
		}
#endif
		return {};
	}

	UniquePtr<Process> Process::get(const char* name)
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
					if (strcmp(entry.szExeFile, name) == 0)
					{
						return soup::make_unique<Process>(entry.th32ProcessID, entry.szExeFile);
					}
				} while (Process32Next(hSnap, &entry));
			}
		}
#else
		for (const auto& entry : std::filesystem::directory_iterator("/proc"))
		{
			if (!entry.is_directory())
			{
				continue;
			}

			const auto optPid = string::toIntOpt<pid_t>(entry.path().filename(), string::TI_FULL);
			if (!optPid)
			{
				continue;
			}

			std::ifstream commFile(entry.path() / "comm");
			if (!commFile.is_open())
			{
				continue;
			}

			std::string procName;
			std::getline(commFile, procName);
			commFile.close();

			if (procName == name)
			{
				return soup::make_unique<Process>(*optPid, std::move(procName));
			}
		}
#endif
		return {};
	}

	std::vector<UniquePtr<Process>> Process::getAll()
	{
		std::vector<UniquePtr<Process>> res{};
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
					res.emplace_back(soup::make_unique<Process>(entry.th32ProcessID, entry.szExeFile));
				} while (Process32Next(hSnap, &entry));
			}
		}
#else
		for (const auto& entry : std::filesystem::directory_iterator("/proc"))
		{
			if (!entry.is_directory())
			{
				continue;
			}

			const auto optPid = string::toIntOpt<pid_t>(entry.path().filename(), string::TI_FULL);
			if (!optPid)
			{
				continue;
			}

			std::ifstream commFile(entry.path() / "comm");
			if (!commFile.is_open())
			{
				continue;
			}

			std::string procName;
			std::getline(commFile, procName);
			commFile.close();

			res.emplace_back(soup::make_unique<Process>(*optPid, std::move(procName)));
		}
#endif
		return res;
	}

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
