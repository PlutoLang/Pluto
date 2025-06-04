#include "ProcessHandle.hpp"

#include "memGuard.hpp"
#include "Range.hpp"

#if SOUP_WINDOWS
	#include <windows.h>
#else
	#include "FileReader.hpp"
	#include "Regex.hpp"
	#include "string.hpp"
#endif

NAMESPACE_SOUP
{
	std::vector<ProcessHandle::AllocationInfo> ProcessHandle::getAllocations() const
	{
		std::vector<ProcessHandle::AllocationInfo> res{};
#if SOUP_WINDOWS
		MEMORY_BASIC_INFORMATION mbi{};

		PBYTE addr = NULL;
		while (VirtualQueryEx(*h, addr, &mbi, sizeof(mbi)) == sizeof(mbi))
		{
			if (mbi.State == MEM_COMMIT)
			{
				res.emplace_back(AllocationInfo{ Range{ mbi.BaseAddress, mbi.RegionSize }, memGuard::protectToAllowedAccess(mbi.Protect) });
			}

			addr = (PBYTE)mbi.BaseAddress + mbi.RegionSize;
		}
#else
		Regex r(R"(^(?'start'[0-9A-Fa-f]+)-(?'end'[0-9A-Fa-f]+) +(?'prots'[a-z\-]+) +[^ ]+ +[^ ]+ +[^ ]+ +(?'mappedfile'.*)$)");
		FileReader fr("/proc/" + std::to_string(pid) + "/maps");
		for (std::string line; fr.getLine(line); )
		{
			auto m = r.match(line);
			/*if (auto mappedfile = m.findGroupByName("mappedfile"))
			{
				if (mappedfile->length() != 0)
				{
					continue;
				}
			}*/
			auto start = string::hexToIntOpt<uintptr_t>(m.findGroupByName("start")->toString()).value();
			auto end = string::hexToIntOpt<uintptr_t>(m.findGroupByName("end")->toString()).value();
			int allowed_access = 0;
			{
				const auto prots = m.findGroupByName("prots")->begin;
				if (prots[0] == 'r')
				{
					allowed_access |= memGuard::ACC_READ;
				}
				if (prots[1] == 'w')
				{
					allowed_access |= memGuard::ACC_WRITE;
				}
				if (prots[2] == 'x')
				{
					allowed_access |= memGuard::ACC_EXEC;
				}
			}
			res.emplace_back(AllocationInfo{ Range{ start, end - start }, allowed_access });
		}
#endif
		return res;
	}
}
