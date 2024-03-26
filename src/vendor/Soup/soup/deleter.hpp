#pragma once

namespace soup
{
	using deleter_t = void(*)(void*);

	template <typename T>
	void deleter_impl(void* ptr)
	{
		delete reinterpret_cast<T*>(ptr);
	}
}
