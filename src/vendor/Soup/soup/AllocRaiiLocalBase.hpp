#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	struct AllocRaiiLocalBase
	{
		void* addr;

		AllocRaiiLocalBase() noexcept
			: addr(nullptr)
		{
		}

		AllocRaiiLocalBase(void* data) noexcept
			: addr(data)
		{
		}

		AllocRaiiLocalBase(const AllocRaiiLocalBase&) = delete;
		AllocRaiiLocalBase(AllocRaiiLocalBase&&) = delete;
	};
}
