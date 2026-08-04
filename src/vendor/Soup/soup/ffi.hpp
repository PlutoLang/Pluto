#pragma once

#include <cstdint>
#include <vector>

#include "Exception.hpp"

NAMESPACE_SOUP
{
	// Raised by ffi::call if nargs > ffi::MAX_CALL_ARGS
	struct BadCall : public Exception
	{
		BadCall()
			: Exception("Bad call")
		{
		}
	};

	struct ffi
	{
		enum ValueType : uint8_t
		{
			VT_INTEGRAL,
			VT_FLOAT,
		};

		constexpr static auto MAX_CALL_ARGS = 20;
		constexpr static auto /*deprecated*/ MAX_ARGS = MAX_CALL_ARGS;
		constexpr static auto MAX_CALLBACK_ARGS = 20;

		[[nodiscard]] static bool isSafeToCall(void* func) noexcept;

#if SOUP_BITS == 32
		using native_float_t = float;
#else
		using native_float_t = double;
#endif

		[[nodiscard]] static uintptr_t reinterpret_float_to_int(native_float_t value)
		{
			return *reinterpret_cast<uintptr_t*>(&value);
		}

		[[nodiscard]] static native_float_t reinterpret_int_to_float(uintptr_t value)
		{
			return *reinterpret_cast<native_float_t*>(&value);
		}

		// types[nargs] is used for the return type.
		static uintptr_t call(void* func, const ValueType types[/*nargs + 1*/], const uintptr_t args[/*nargs*/], size_t nargs);

#define SOUP_FFI_CALLBACK_AVAILABLE (SOUP_X86 || (SOUP_ARM && SOUP_BITS == 64))
#if SOUP_FFI_CALLBACK_AVAILABLE
		// Returns nullptr on allocation failure.
		// On MacOS, allocation may fail if the 'com.apple.security.cs.allow-jit' entitlement is missing.
		[[nodiscard]] static void* callbackAlloc(uintptr_t(*func)(uintptr_t user_data, const uintptr_t args[MAX_CALLBACK_ARGS]), uintptr_t user_data) noexcept;
		static void callbackFree(void* cb) noexcept;
#endif
	};
}
