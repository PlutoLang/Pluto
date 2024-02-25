#pragma once

#include <cstddef>
#include <cstdint>

// === Determine platform

#ifdef _WIN32
	#define SOUP_WINDOWS true
	#define SOUP_WASM false
	#define SOUP_LINUX false
	#define SOUP_POSIX false
	#define SOUP_MACOS false
	#define SOUP_ANDROID false

	#define SOUP_EXPORT __declspec(dllexport)

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
#else
	#define SOUP_WINDOWS false
	#define SOUP_POSIX true

	#ifdef __EMSCRIPTEN__
		#define SOUP_WASM true
		#define SOUP_LINUX false
		#define SOUP_MACOS false

		#include "emscripten.h"
		#define SOUP_EXPORT EMSCRIPTEN_KEEPALIVE
	#else
		#define SOUP_WASM false

		#if defined(__linux__) && !defined(__ANDROID__)
			#define SOUP_LINUX true
		#else
			#define SOUP_LINUX false
		#endif

		#ifdef __ANDROID__
			#define SOUP_ANDROID true
		#else
			#define SOUP_ANDROID false
		#endif

		#if defined(__APPLE__) && defined(__MACH__)
			#define SOUP_MACOS true
		#else
			#define SOUP_MACOS false
		#endif

		#define SOUP_EXPORT __attribute__ ((visibility ("default")))
	#endif
#endif

// === Determine CPU register size

#if defined(__x86_64__) || defined(_M_X64) || defined(__ppc64__) || defined(__aarch64__)
	#define SOUP_BITS 64
#else
	#define SOUP_BITS 32
#endif

// === Determine CPU architecture

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	#define SOUP_X86 true
#else
	#define SOUP_X86 false
#endif

// === Determine if code inspector

#ifdef __INTELLISENSE__
	#define SOUP_CODE_INSPECTOR true
#else
	#define SOUP_CODE_INSPECTOR false
#endif

// === Platform abstraction macros

#define SOUP_CEXPORT extern "C" SOUP_EXPORT

#ifdef _MSC_VER
	#define SOUP_FORCEINLINE __forceinline
	#define SOUP_NOINLINE __declspec(noinline)
#else
	#define SOUP_FORCEINLINE __attribute__((always_inline))
	#define SOUP_NOINLINE __attribute__((noinline))
#endif

// === C++ version abstraction macros

#if __cplusplus == 1997'11L
	#error Please set the /Zc:__cplusplus compiler flag or manually adjust __cplusplus when using Soup.
#endif

#ifdef SOUP_CPP20
	#pragma message("Ignoring SOUP_CPP20 define, this is automatically set based on C++ version.")
	#undef SOUP_CPP20
#endif

#if __cplusplus < 2020'00L
	#define SOUP_CPP20 false
#else
	#define SOUP_CPP20 true
#endif

#if SOUP_CPP20
	#define SOUP_CONSTEVAL consteval
	#define SOUP_CONSTEXPR20 constexpr
#else
	#define SOUP_CONSTEVAL constexpr
	#define SOUP_CONSTEXPR20
#endif

#if SOUP_CPP20
	#define SOUP_IF_LIKELY(cond) if (cond) [[likely]]
	#define SOUP_IF_UNLIKELY(cond) if (cond) [[unlikely]]
#else
	#define SOUP_IF_LIKELY(cond) if (cond)
	#define SOUP_IF_UNLIKELY(cond) if (cond)
#endif

#if __cplusplus < 2023'00L
	#define SOUP_CPP23 false
#else
	#define SOUP_CPP23 true
#endif

#if SOUP_CPP23
	#define SOUP_ASSUME(x) [[assume(x)]];
	#define SOUP_UNREACHABLE std::unreachable();
#else
	#define SOUP_ASSUME(x) ;
	#define SOUP_UNREACHABLE ;
#endif

// === C++ feature abstraction macros

#if (__cpp_exceptions < 1997'11L) && (!defined(_MSC_VER) || defined(__clang__))
	#define SOUP_EXCEPTIONS false
	#define SOUP_THROW(x) ::soup::throwImpl(x);

	#include <stdexcept>

	namespace soup
	{
		[[noreturn]] void throwImpl(std::exception&& e);
	}
#else
	#define SOUP_EXCEPTIONS true
	#define SOUP_THROW(x) throw x;
#endif

#ifndef SOUP_EXCAL
	// An 'excal' function is 'noexcept' except it may throw std::bad_alloc.
	//
	// We generally don't attempt to handle allocation failures, not least because it's basically impossible on modern systems.
	// Because of this, we declare that 'excal' functions are 'noexcept' to avoid superfluous unwind information.
	//
	// For visual distinction with IDE hover features, we use `throw()`, but it's functionally identical to `noexcept`.
	#define SOUP_EXCAL throw()
#endif

// === Platform-specific types

namespace soup
{
#if SOUP_BITS == 64
	using halfintmax_t = int32_t;
	using halfsize_t = uint32_t;
#elif SOUP_BITS == 32
	using halfintmax_t = int16_t;
	using halfsize_t = uint16_t;
#endif
}

// === Development helper macros

namespace soup
{
	[[nodiscard]] void* malloc(size_t size) /* SOUP_EXCAL */;
	[[nodiscard]] void* realloc(void* ptr, size_t new_size) /* SOUP_EXCAL */;

	[[noreturn]] void throwAssertionFailed();
	[[noreturn]] void throwAssertionFailed(const char* what);
}
#define SOUP_ASSERT(x, ...) SOUP_IF_UNLIKELY (!(x)) { ::soup::throwAssertionFailed(__VA_ARGS__); }
#define SOUP_ASSERT_UNREACHABLE ::soup::throwAssertionFailed();

#ifndef NDEBUG
	#define SOUP_DEBUG_ASSERT(x) SOUP_ASSERT(x)
	#define SOUP_DEBUG_ASSERT_UNREACHABLE SOUP_UNREACHABLE
#else
	#define SOUP_DEBUG_ASSERT(x) SOUP_ASSUME(X)
	#define SOUP_DEBUG_ASSERT_UNREACHABLE SOUP_ASSERT_UNREACHABLE
#endif

template <typename T> SOUP_FORCEINLINE void SOUP_UNUSED(T&&) {}

// RVO may not apply when returning the member of a class-instance in the function's scope.
// In that case, one could do `return std::move(inst.member);` to move out the return value.
// However, if we have a flat return value, `return std::move(flat);` could be a pessimisation.
// This macro ensures the return value is moved out in both cases with zero overhead.
#define SOUP_MOVE_RETURN(x) auto rvoable_return_value = std::move(x); return rvoable_return_value;
