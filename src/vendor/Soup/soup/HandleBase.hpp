#pragma once

#include <windows.h>

NAMESPACE_SOUP
{
	struct HandleBase
	{
		HANDLE h = INVALID_HANDLE_VALUE;

		HandleBase() noexcept = default;
		HandleBase(const HandleBase&) = delete;

		HandleBase(HANDLE h) noexcept
			: h(h)
		{
		}

		void operator=(const HandleBase&) = delete;

		void operator=(HandleBase&& b) noexcept
		{
			this->~HandleBase();
			h = b.h;
			b.h = INVALID_HANDLE_VALUE;
		}

		void operator=(HANDLE h) noexcept
		{
			this->~HandleBase();
			this->h = h;
		}

		void set(HANDLE h) noexcept
		{
			this->~HandleBase();
			this->h = h;
		}

		virtual ~HandleBase() noexcept
		{
		}

		[[nodiscard]] operator HANDLE() const noexcept
		{
			return h;
		}

		template <typename T>
		[[nodiscard]] operator T() const noexcept
		{
			return (T)h;
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return isValid();
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
}
