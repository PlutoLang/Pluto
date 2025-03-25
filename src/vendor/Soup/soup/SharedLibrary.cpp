#include "SharedLibrary.hpp"

#include <string>

#include "Exception.hpp"

NAMESPACE_SOUP
{
	SharedLibrary::SharedLibrary(const char* path) noexcept
	{
		load(path);
	}

	SharedLibrary::SharedLibrary(SharedLibrary&& b) noexcept
		: handle(b.handle)
	{
		b.forget();
	}

	SharedLibrary::~SharedLibrary() noexcept
	{
		unload();
	}

	void SharedLibrary::operator=(SharedLibrary&& b) noexcept
	{
		unload();
		handle = b.handle;
		b.forget();
	}

	bool SharedLibrary::isLoaded() const noexcept
	{
		return handle != nullptr;
	}

	bool SharedLibrary::load(const char* path) noexcept
	{
#if SOUP_WINDOWS
		handle = LoadLibraryA(path);
#else
		handle = dlopen(path, RTLD_LAZY);
#endif
		return isLoaded();
	}

	void SharedLibrary::unload() noexcept
	{
		if (isLoaded())
		{
#if SOUP_WINDOWS
			FreeLibrary(handle);
#else
			dlclose(handle);
#endif
			forget();
		}
	}

	void SharedLibrary::forget() noexcept
	{
		handle = nullptr;
	}

	void* SharedLibrary::getAddress(const char* name) const noexcept
	{
		SOUP_IF_UNLIKELY (!isLoaded())
		{
			return nullptr;
		}
#if SOUP_WINDOWS
		return (void*)GetProcAddress(handle, name);
#else
		return dlsym(handle, name);
#endif
	}

	void* SharedLibrary::getAddressMandatory(const char* name) const
	{
		SOUP_IF_LIKELY (auto addr = getAddress(name))
		{
			return addr;
		}
		std::string msg = "Failed to find mandatory symbol: ";
		msg.append(name);
		SOUP_THROW(Exception(std::move(msg)));
	}
}
