#pragma once

#include "base.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <unistd.h> // close
#endif

NAMESPACE_SOUP
{
#if SOUP_WINDOWS
	struct HandleRaii
	{
		HANDLE h = INVALID_HANDLE_VALUE;

		HandleRaii() noexcept = default;

		HandleRaii(HANDLE h) noexcept
			: h(h)
		{
		}

		HandleRaii(HandleRaii&& b) noexcept
			: h(b.h)
		{
			b.h = INVALID_HANDLE_VALUE;
		}

		~HandleRaii() noexcept
		{
			if (isValid())
			{
				CloseHandle(h);
			}
		}

		void operator=(HANDLE h) noexcept
		{
			this->~HandleRaii();
			this->h = h;
		}

		void operator=(HandleRaii&& b) noexcept
		{
			this->~HandleRaii();
			this->h = b.h;
			b.h = INVALID_HANDLE_VALUE;
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return isValid();
		}

		[[nodiscard]] operator HANDLE() const noexcept
		{
			return h;
		}

		[[nodiscard]] bool isValid() const noexcept
		{
			return h != INVALID_HANDLE_VALUE;
		}

		void invalidate() noexcept
		{
			h = INVALID_HANDLE_VALUE;
		}
	};
#else
	struct HandleRaii
	{
		int handle = -1;

		HandleRaii() noexcept = default;

		HandleRaii(int handle) noexcept
			: handle(handle)
		{
		}

		HandleRaii(HandleRaii&& b) noexcept
			: handle(b.handle)
		{
			b.handle = -1;
		}

		~HandleRaii() noexcept
		{
			if (isValid())
			{
				::close(handle);
			}
		}

		[[nodiscard]] bool isValid() const noexcept
		{
			return handle >= 0;
		}

		void operator=(int handle) noexcept
		{
			this->~HandleRaii();
			this->handle = handle;
		}

		void operator=(HandleRaii&& b) noexcept
		{
			this->~HandleRaii();
			this->handle = b.handle;
			b.handle = -1;
		}

		operator int() const noexcept
		{
			return handle;
		}
	};
#endif
}
