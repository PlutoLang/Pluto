#pragma once

#include "base.hpp"
#include "TransientToken.hpp"

NAMESPACE_SOUP
{
	// To make your structs weakref-able, simply add this property -> soup::TransientToken transient_token;
	template <class T>
	class WeakRef
	{
	private:
		TransientTokenRef tt;
	public:
		T* ptr;

		WeakRef()
			: tt(false), ptr(nullptr)
		{
		}

		WeakRef(T* ptr)
			: WeakRef(ptr->transient_token, ptr)
		{
		}

		WeakRef(const TransientTokenBase& tt, T* ptr)
			: tt(tt), ptr(ptr)
		{
		}

	private:
		WeakRef(bool valid, T* ptr)
			: tt(valid), ptr(ptr)
		{
		}

	public:
		[[nodiscard]] static WeakRef makeValidNullptrRef()
		{
			return WeakRef(true, nullptr);
		}

		[[nodiscard]] bool isValid() const noexcept
		{
			return tt.isValid();
		}

		[[nodiscard]] T* getPointer() const noexcept
		{
			// return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) * isValid());
			SOUP_IF_LIKELY (isValid())
			{
				return ptr;
			}
			return nullptr;
		}

		operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		void reset() SOUP_EXCAL
		{
			tt.reset();
			ptr = nullptr;
		}
	};
}
