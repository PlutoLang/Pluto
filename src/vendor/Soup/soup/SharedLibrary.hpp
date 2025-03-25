#pragma once

#include "base.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "type_traits.hpp"

NAMESPACE_SOUP
{
	struct SharedLibrary
	{
#if SOUP_WINDOWS
		using handle_t = HMODULE;
#else
		using handle_t = void*;
#endif

		handle_t handle = nullptr;

		explicit SharedLibrary() noexcept = default;
		explicit SharedLibrary(const char* path) noexcept;
		explicit SharedLibrary(SharedLibrary&& b) noexcept;
		~SharedLibrary() noexcept;

		void operator=(SharedLibrary&& b) noexcept;

		[[nodiscard]] bool isLoaded() const noexcept;
		bool load(const char* path) noexcept;
		void unload() noexcept;
		void forget() noexcept;

		template <typename T, SOUP_RESTRICT(std::is_pointer_v<T>)>
		[[nodiscard]] T getAddress(const char* name) const noexcept
		{
			return reinterpret_cast<T>(getAddress(name));
		}

		template <typename T, SOUP_RESTRICT(std::is_pointer_v<T>)>
		[[nodiscard]] T getAddressMandatory(const char* name) const noexcept
		{
			return reinterpret_cast<T>(getAddressMandatory(name));
		}

		[[nodiscard]] void* getAddress(const char* name) const noexcept;
		[[nodiscard]] void* getAddressMandatory(const char* name) const;
	};
}
