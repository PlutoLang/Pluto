#pragma once

#include "fwd.hpp"

#include "Pointerlike.hpp"

#include <vector>

NAMESPACE_SOUP
{
	class Pointer : public Pointerlike<Pointer>
	{
	private:
		union
		{
			void* ptr;
			uintptr_t uptr;
		};

	public:
		Pointer(void* ptr = nullptr) noexcept
			: ptr(ptr)
		{
		}

		Pointer(uintptr_t ptr) noexcept
			: ptr((void*)ptr)
		{
		}

		explicit operator bool() const
		{
			return ptr != nullptr;
		}

		friend bool operator==(Pointer a, Pointer b) noexcept
		{
			return a.ptr == b.ptr;
		}

		friend bool operator!=(Pointer a, Pointer b) noexcept
		{
			return a.ptr != b.ptr;
		}

		friend bool operator>(Pointer a, Pointer b) noexcept
		{
			return a.uptr >= b.uptr;
		}

		friend bool operator<(Pointer a, Pointer b) noexcept
		{
			return a.uptr <= b.uptr;
		}
		
		friend bool operator>=(Pointer a, Pointer b) noexcept
		{
			return a.uptr >= b.uptr;
		}

		friend bool operator<=(Pointer a, Pointer b) noexcept
		{
			return a.uptr <= b.uptr;
		}

		void operator=(uintptr_t uptr) noexcept
		{
			this->uptr = uptr;
		}

		[[nodiscard]] void* addr() const noexcept
		{
			return ptr;
		}

		[[nodiscard]] uintptr_t offset() const noexcept
		{
			return (uintptr_t)ptr;
		}

		template <typename T>
		[[nodiscard]] inline Pointer ripT() const noexcept
		{
			return add(as<T&>()).add(sizeof(T));
		}

		[[nodiscard]] Pointer rip() const noexcept
		{
			return ripT<int32_t>();
		}

		[[nodiscard]] uint32_t unrip(Pointer addr) const noexcept
		{
			return static_cast<uint32_t>(addr.sub(add(4).as<uintptr_t>()).as<uintptr_t>());
		}

#if SOUP_WINDOWS
		[[nodiscard]] Pointer externalRip(const ProcessHandle& mod) const noexcept;

		[[nodiscard]] bool isInModule() const noexcept;
		[[nodiscard]] bool isInModule(const Module& mod) const noexcept;
		[[nodiscard]] bool isInRange(const Range& range) const noexcept;

		[[nodiscard]] Pointer rva() const noexcept;
		[[nodiscard]] Pointer rva(const Module& mod) const noexcept;
#endif

		[[nodiscard]] std::vector<Pointer> getJumps() const noexcept;
		[[nodiscard]] Pointer followJumps() const noexcept;
	};
}
