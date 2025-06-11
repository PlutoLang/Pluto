#pragma once

// === Namespace

// When vendoring Soup, it is advisable to assign a unique namespace, like so:
//
// namespace soup { namespace PROJECTNAME_vendored {}; using namespace PROJECTNAME_vendored; };
// #define NAMESPACE_SOUP namespace soup::PROJECTNAME_vendored
//
// Note that this does not change the way you use Soup.

namespace soup { namespace pluto_vendored {}; using namespace pluto_vendored; };
#define NAMESPACE_SOUP namespace soup::pluto_vendored

// === Platform/ABI macros

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

	#ifdef __MINGW32__
		#define SOUP_CROSS_COMPILE true
		#if _WIN32_WINNT < 0x600
			#undef _WIN32_WINNT
			#define _WIN32_WINNT 0x600
		#endif
	#else
		#define SOUP_CROSS_COMPILE false
	#endif
#else
	#define SOUP_WINDOWS false
	#define SOUP_POSIX true
	#define SOUP_CROSS_COMPILE false

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

		#ifdef __APPLE__
			#define SOUP_APPLE true
		#else
			#define SOUP_APPLE false
		#endif

		#if defined(__APPLE__) && defined(__MACH__)
			#define SOUP_MACOS true
		#else
			#define SOUP_MACOS false
		#endif

		#define SOUP_EXPORT __attribute__ ((visibility ("default")))
	#endif
#endif

#define SOUP_CEXPORT extern "C" SOUP_EXPORT

#if defined(_MSC_VER) && !defined(__clang__)
	#define SOUP_FORCEINLINE __forceinline
	#define SOUP_NOINLINE __declspec(noinline)
	#define SOUP_PURE __declspec(noalias)
	#define SOUP_UNIQADDR __restrict
#else
	#define SOUP_FORCEINLINE __attribute__((always_inline)) inline
	#define SOUP_NOINLINE __attribute__((noinline))
	#define SOUP_PURE __attribute__((pure))
	#define SOUP_UNIQADDR __restrict__
#endif

// === CPU macros

#if defined(__x86_64__) || defined(_M_X64) || defined(__ppc64__) || defined(__aarch64__) || defined(_M_ARM64)
	#define SOUP_BITS 64
#else
	#define SOUP_BITS 32
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	#define SOUP_X86 true
#else
	#define SOUP_X86 false
#endif

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
	#define SOUP_ARM true
	#if defined(__clang_major__) && __clang_major__ <= 14 && !defined(__ARM_FEATURE_CRYPTO)
		#error Your version of Clang predates some important fixes for ARM codegen. Provide -march=armv8+crypto+crc to compile Soup. Note that the resulting binary may be incorrect.
	#endif
#else
	#define SOUP_ARM false
#endif

// === Determine if code inspector

#ifdef __INTELLISENSE__
	#define SOUP_CODE_INSPECTOR true
#else
	#define SOUP_CODE_INSPECTOR false
#endif

// === C++ conditional feature macros

#ifdef _MSVC_LANG
	#define SOUP_CPP_VERSION _MSVC_LANG
#else
	#define SOUP_CPP_VERSION __cplusplus
#endif

#if SOUP_CPP_VERSION < 2020'00L
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
	#define SOUP_IF_LIKELY(...) if (__VA_ARGS__) [[likely]]
	#define SOUP_IF_UNLIKELY(...) if (__VA_ARGS__) [[unlikely]]
#else
	#define SOUP_IF_LIKELY(...) if (__VA_ARGS__)
	#define SOUP_IF_UNLIKELY(...) if (__VA_ARGS__)
#endif

#if SOUP_CPP_VERSION < 2023'00L
	#define SOUP_CPP23 false
#else
	#define SOUP_CPP23 true
#endif

#if SOUP_CPP23
	#define SOUP_ASSUME(...) [[assume(__VA_ARGS__)]];
	#define SOUP_UNREACHABLE std::unreachable();
#else
	#if defined(_MSC_VER) && !defined(__clang__)
		#define SOUP_ASSUME(...) __assume(__VA_ARGS__);
		#define SOUP_UNREACHABLE SOUP_ASSUME(false);
	#else
		#define SOUP_ASSUME(...) ;
		#define SOUP_UNREACHABLE __builtin_unreachable();
	#endif
#endif

#if (__cpp_exceptions < 1997'11L) && (!defined(_MSC_VER) || defined(__clang__))
	#define SOUP_EXCEPTIONS false
	#define SOUP_TRY if (true)
	#define SOUP_CATCH(T, name) if (const T& name = *(T*)nullptr; false)
	#define SOUP_CATCH_ANY if (false)
	#define SOUP_THROW(x) ::soup::throwImpl(x);

	#include <stdexcept>

	NAMESPACE_SOUP
	{
		[[noreturn]] void throwImpl(std::exception&& e);
	}
#else
	#define SOUP_EXCEPTIONS true
	#define SOUP_TRY try
	#define SOUP_CATCH(T, name) catch (const T& name)
	#define SOUP_CATCH_ANY catch (...)
	#define SOUP_THROW(x) throw x;
#endif

#ifndef SOUP_EXCAL
	// An 'excal' function may not throw any exception other than std::bad_alloc (in case of an allocation failure).
	//
	// If you don't handle allocate failures, you can slightly reduce your binary size by setting this macro to `noexcept` or `throw()`.
	#define SOUP_EXCAL
#endif

// === Development helpers

NAMESPACE_SOUP
{
	[[noreturn]] void throwAssertionFailed();
	[[noreturn]] void throwAssertionFailed(const char* what);
}
#define SOUP_ASSERT(x, ...) SOUP_IF_UNLIKELY (!(x)) { ::soup::throwAssertionFailed(__VA_ARGS__); }
#define SOUP_ASSERT_UNREACHABLE ::soup::throwAssertionFailed();

#ifndef NDEBUG
	#define SOUP_DEBUG_ASSERT(x) SOUP_ASSERT(x)
	#define SOUP_DEBUG_ASSERT_UNREACHABLE SOUP_UNREACHABLE
#else
	#define SOUP_DEBUG_ASSERT(x) SOUP_ASSUME(x)
	#define SOUP_DEBUG_ASSERT_UNREACHABLE SOUP_ASSERT_UNREACHABLE
#endif

template <typename T> SOUP_FORCEINLINE void SOUP_UNUSED(T&&) {}

#define SOUP_RETHROW_FALSE(x) SOUP_IF_UNLIKELY (!(x)) { return {}; }

// Enable compiler warning for unannotated fallthroughs
#if defined(__clang__)
#pragma clang diagnostic warning "-Wimplicit-fallthrough"
//#elif defined(_MSC_VER)
//#pragma warning(default: 5262) // MSVC is too retarded, it thinks everything is an implicit fallthrough.
#elif defined(__GNUC__)
#pragma GCC diagnostic warning "-Wimplicit-fallthrough"
#endif
