#pragma once

#include <cstdint>
#include <vector>

#include "Exception.hpp"

NAMESPACE_SOUP
{
	// Raised if nargs > ffi::MAX_ARGS
	struct BadCall : public Exception
	{
		BadCall()
			: Exception("Bad call")
		{
		}
	};

	struct ffi
	{
		constexpr static auto MAX_ARGS = 20;

		[[nodiscard]] static bool isSafeToCall(void* func) noexcept;

		static uintptr_t call(void* func, const uintptr_t* args, size_t nargs);

		static uintptr_t call(void* func, const std::vector<uintptr_t>& args)
		{
			return call(func, args.data(), args.size());
		}

		[[deprecated]] static uintptr_t fastcall(void* func, const std::vector<uintptr_t>& args)
		{
			return call(func, args);
		}

#if SOUP_X86 && SOUP_BITS == 64
		[[nodiscard]] static bool callbackAvailable() { return true; }
#else
		[[nodiscard]] static bool callbackAvailable() { return false; }
#endif
		[[nodiscard]] static void* callbackAlloc(uintptr_t(*func)(uintptr_t user_data, const uintptr_t* args), uintptr_t user_data);
		static void callbackFree(void* cb);
	};
}
