#pragma once

#include "base.hpp"
#if SOUP_WINDOWS
#include "HandleBase.hpp"

NAMESPACE_SOUP
{
	struct VirtualHandleRaii final : public HandleBase
	{
		using HandleBase::HandleBase;

		VirtualHandleRaii(HANDLE h) noexcept
			: HandleBase(h)
		{
		}

		VirtualHandleRaii(VirtualHandleRaii&& b) noexcept
			: HandleBase(b.h)
		{
			b.h = INVALID_HANDLE_VALUE;
		}

		~VirtualHandleRaii() noexcept final
		{
			if (isValid())
			{
				CloseHandle(h);
			}
		}

		using HandleBase::operator=;
	};
}
#endif
