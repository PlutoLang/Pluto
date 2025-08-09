#pragma once

#include "base.hpp"
#include "type.hpp" // pid_t

#include <memory>
#include <vector>
#if SOUP_WINDOWS
	#include <string>

	#include <windows.h>
#endif

#include "ProcessHandle.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	class Process
	{
	public:
		const pid_t id;
#if SOUP_WINDOWS
		const std::string name;
#endif

#if SOUP_WINDOWS
		Process(pid_t id, std::string&& name)
			: id(id), name(std::move(name))
		{
		}
#else
		Process(pid_t id)
			: id(id)
		{
		}
#endif

		[[nodiscard]] static UniquePtr<Process> current();

		[[nodiscard]] static UniquePtr<Process> get(pid_t id);
#if SOUP_WINDOWS
		[[nodiscard]] static UniquePtr<Process> get(const char* name);
		[[nodiscard]] static std::vector<UniquePtr<Process>> getAll();
#endif

		[[nodiscard]] std::shared_ptr<ProcessHandle> open() const;
	};
}
