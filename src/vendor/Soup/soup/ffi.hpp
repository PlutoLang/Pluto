#pragma once

#include <cstdint>
#include <vector>

#include "Exception.hpp"

NAMESPACE_SOUP
{
	// Raised by ffi::call if nargs > ffi::MAX_CALL_ARGS or nfixedargs > nargs
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

		constexpr static size_t MAX_CALL_ARGS = 20;
		constexpr static size_t /*deprecated*/ MAX_ARGS = MAX_CALL_ARGS;
		constexpr static size_t MAX_CALLBACK_ARGS = 20;

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
		static uintptr_t call(void* func, ValueType types[/*nargs + 1*/], const uintptr_t args[/*nargs*/], size_t nargs);

		// For variadic calls like `printf("%d:%d", 1, 2);`; in this example there's 1 fixed arg and 3 args total.
		static uintptr_t callVariadic(void* func, ValueType types[/*nargs + 1*/], const uintptr_t args[/*nargs*/], size_t nfixedargs, size_t nargs);

#define SOUP_FFI_CALLBACK_AVAILABLE (SOUP_X86 || (SOUP_ARM && SOUP_BITS == 64))
#if SOUP_FFI_CALLBACK_AVAILABLE
		using callback_t = uintptr_t(*)(uintptr_t user_data, const uintptr_t args[MAX_CALLBACK_ARGS]);

		// Returns nullptr on allocation failure. On MacOS, allocation may fail if the 'com.apple.security.cs.allow-jit' entitlement is missing.
		// types[MAX_CALLBACK_ARGS] is used for the return type.
		[[nodiscard]] static void* callbackAlloc(callback_t func, uintptr_t user_data, const ValueType types[MAX_CALLBACK_ARGS + 1]) noexcept;
		static void callbackFree(void* cb) noexcept;
#endif
	};

#if SOUP_MACOS && SOUP_ARM
	inline uintptr_t ffi::call(void* func, ValueType types[/*nargs + 1*/], const uintptr_t args[/*nargs*/], size_t nargs)
	{
		return callVariadic(func, types, args, nargs, nargs);
	}
#elif !SOUP_WINDOWS
	inline uintptr_t ffi::callVariadic(void* func, ValueType types[/*nargs + 1*/], const uintptr_t args[/*nargs*/], size_t nfixedargs, size_t nargs)
	{
		return call(func, types, args, nargs);
	}
#endif

}
