#pragma once

#include "base.hpp"

#define DELAYED_CTOR_DEBUGGER_FRIENDLY false

#if DELAYED_CTOR_DEBUGGER_FRIENDLY
#include "UniquePtr.hpp"
#else
#include <memory> // destroy_at
#include <utility> // forward
#include "memory.hpp" // construct_at
#endif

NAMESPACE_SOUP
{
	template <typename T>
	class alignas(T) DelayedCtor
	{
	private:
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
		UniquePtr<T> data{};
#else
		char buf[sizeof(T)];
		bool constructed;
#endif

	public:
		DelayedCtor()
#if !DELAYED_CTOR_DEBUGGER_FRIENDLY
			: constructed(false)
#endif
		{
		}

		DelayedCtor(T&&) = delete;
		DelayedCtor(const T&) = delete;
		DelayedCtor(DelayedCtor&&) = delete;
		DelayedCtor(const DelayedCtor&) = delete;

		~DelayedCtor()
		{
#if !DELAYED_CTOR_DEBUGGER_FRIENDLY
			if (constructed)
			{
				std::destroy_at<>(reinterpret_cast<T*>(&buf[0]));
			}
#endif
		}

		[[nodiscard]] bool isConstructed() const noexcept
		{
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			return data;
#else
			return constructed;
#endif
		}

		template <typename...Args>
		T* construct(Args&&...args)
		{
			SOUP_ASSERT(!isConstructed());
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			data = soup::make_unique<T, Args...>(std::forward<Args>(args)...);
			return data.get();
#else
			soup::construct_at<T, Args...>(reinterpret_cast<T*>(&buf[0]), std::forward<Args>(args)...);
			constructed = true;
			return reinterpret_cast<T*>(&buf[0]);
#endif
		}

		void destroy()
		{
			SOUP_ASSERT(isConstructed());
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			data.reset();
#else
			std::destroy_at<>(reinterpret_cast<T*>(&buf[0]));
			constructed = false;
#endif
		}

		void reset()
		{
			if (isConstructed())
			{
				destroy();
			}
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return isConstructed();
		}

		[[nodiscard]] T& operator*() noexcept
		{
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			return *data;
#else
			return *reinterpret_cast<T*>(&buf[0]);
#endif
		}

		[[nodiscard]] const T& operator*() const noexcept
		{
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			return *data;
#else
			return *reinterpret_cast<const T*>(&buf[0]);
#endif
		}

		[[nodiscard]] T* operator->() noexcept
		{
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			return data.get();
#else
			return reinterpret_cast<T*>(&buf[0]);
#endif
		}

		[[nodiscard]] const T* operator->() const noexcept
		{
#if DELAYED_CTOR_DEBUGGER_FRIENDLY
			return data.get();
#else
			return reinterpret_cast<const T*>(&buf[0]);
#endif
		}
	};
}
