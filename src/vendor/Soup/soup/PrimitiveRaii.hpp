#pragma once

#include "IntStruct.hpp"

NAMESPACE_SOUP
{
	template <typename T, T InvalidValue = 0>
	struct PrimitiveRaii : public IntStruct<T>
	{
		PrimitiveRaii(T val = InvalidValue) noexcept
			: IntStruct<T>(val)
		{
		}
		
		PrimitiveRaii(PrimitiveRaii&& b) noexcept
			: IntStruct<T>(b)
		{
			b.setMovedAway();
		}

		void operator=(PrimitiveRaii&& b) noexcept
		{
			IntStruct<T>::data = b.data;
			b.setMovedAway();
		}

		void setMovedAway() noexcept
		{
			IntStruct<T>::data = InvalidValue;
		}
	};
}
