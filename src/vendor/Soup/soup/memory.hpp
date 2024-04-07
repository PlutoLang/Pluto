#pragma once

#include <utility> // forward

#include "base.hpp"

NAMESPACE_SOUP
{
	// Like std::construct_at except it doesn't require C++ 20
	template <typename T, typename...Args>
	/*constexpr*/ T* construct_at(T* addr, Args&&...args)
		noexcept(noexcept(::new(addr) T(std::forward<Args>(args)...)))
	{
		return ::new(addr) T(std::forward<Args>(args)...);
	}
}
