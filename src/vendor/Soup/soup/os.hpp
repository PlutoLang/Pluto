#pragma once

#include "base.hpp"
#include "fwd.hpp"

#include <filesystem>
#include <string>
#include <vector>

#if SOUP_WINDOWS
#include <Windows.h>
#include <Winternl.h>
#endif

namespace soup
{
	class os
	{
	public:
		[[nodiscard]] static void* createFileMapping(std::filesystem::path path, size_t& out_len);
		static void destroyFileMapping(void* addr, size_t len);
	};
}
