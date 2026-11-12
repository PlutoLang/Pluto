#pragma once

#include "AllocRaiiLocalBase.hpp"

#include <cstdlib>

NAMESPACE_SOUP
{
	struct AllocRaii : public AllocRaiiLocalBase
	{
		using AllocRaiiLocalBase::AllocRaiiLocalBase;

		AllocRaii(size_t size)
			: AllocRaiiLocalBase(malloc(size))
		{
		}

		AllocRaii(AllocRaiiLocalBase&& b) noexcept
			: AllocRaiiLocalBase(b.addr)
		{
			b.addr = nullptr;
		}

		~AllocRaii() noexcept
		{
			release();
		}

		void release() noexcept
		{
			if (addr != nullptr)
			{
				free(addr);
				addr = nullptr;
			}
		}

		void operator=(AllocRaii&& b) noexcept
		{
			release();
			addr = b.addr;
			b.addr = nullptr;
		}

		operator bool() const noexcept
		{
			return addr != nullptr;
		}

		operator void* () const noexcept
		{
			return addr;
		}
	};
}
