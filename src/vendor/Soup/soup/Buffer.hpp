#pragma once

#include <cstdint>
#include <cstring> // memcpy
#include <string>

#ifndef SOUP_BUFFER_NO_RESIZE
#define SOUP_BUFFER_NO_RESIZE false
#endif

#include "base.hpp"
#if SOUP_BUFFER_NO_RESIZE
#include "Exception.hpp"
#endif
#include "memAllocating.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	// This extremely simple implementation outperforms std::vector & std::string, barring magic optimisations, e.g. clang with std::string::push_back.
	template <typename AllocatorT = void>
	class Buffer : public memAllocating<AllocatorT>
	{
	private:
		uint8_t* m_data = nullptr;
		size_t m_size = 0;
		size_t m_capacity = 0;
	public:
#if SOUP_BUFFER_NO_RESIZE
		bool no_resize = false;
#endif

		Buffer() noexcept = default;

		using memAllocating<AllocatorT>::memAllocating;

		Buffer(const Buffer<AllocatorT>& b) SOUP_EXCAL
		{
			append(b);
		}

		template <typename OtherAllocatorT, SOUP_RESTRICT(std::is_same_v<AllocatorT, OtherAllocatorT> && std::is_void_v<AllocatorT>)>
		Buffer(const Buffer<OtherAllocatorT>& b) SOUP_EXCAL
		{
			append(b);
		}

		template <typename OtherAllocatorT, SOUP_RESTRICT(std::is_same_v<AllocatorT, OtherAllocatorT> && !std::is_void_v<AllocatorT>)>
		Buffer(const Buffer<OtherAllocatorT>& b) SOUP_EXCAL
			: memAllocating<AllocatorT>(b.allocator)
		{
			append(b);
		}

		Buffer(const std::string& b) SOUP_EXCAL
		{
			append(b);
		}

		Buffer(Buffer<AllocatorT>&& b) noexcept
			: m_data(b.m_data), m_size(b.m_size), m_capacity(b.m_capacity)
		{
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		template <typename OtherAllocatorT, SOUP_RESTRICT(std::is_same_v<AllocatorT, OtherAllocatorT> && std::is_void_v<AllocatorT>)>
		Buffer(Buffer<OtherAllocatorT>&& b) noexcept
			: m_data(b.m_data), m_size(b.m_size), m_capacity(b.m_capacity)
		{
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		template <typename OtherAllocatorT, SOUP_RESTRICT(std::is_same_v<AllocatorT, OtherAllocatorT> && !std::is_void_v<AllocatorT>)>
		Buffer(Buffer<OtherAllocatorT>&& b) noexcept
			: memAllocating<AllocatorT>(b.allocator), m_data(b.m_data), m_size(b.m_size), m_capacity(b.m_capacity)
		{
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		~Buffer() noexcept
		{
			if (m_data != nullptr)
			{
				this->deallocate(m_data);
			}
		}

		void operator=(const Buffer<AllocatorT>& b) SOUP_EXCAL
		{
			clear();
			append(b);
		}

		template <typename OtherAllocatorT>
		void operator=(const Buffer<OtherAllocatorT>& b) SOUP_EXCAL
		{
			clear();
			append(b);
		}

		void operator=(Buffer<AllocatorT>&& b) noexcept
		{
			if (m_data != nullptr)
			{
				this->deallocate(m_data);
			}
			this->m_data = b.m_data;
			this->m_size = b.m_size;
			this->m_capacity = b.m_capacity;
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		template <typename OtherAllocatorT, SOUP_RESTRICT(std::is_same_v<AllocatorT, OtherAllocatorT> && std::is_void_v<AllocatorT>)>
		void operator=(Buffer<OtherAllocatorT>&& b) noexcept
		{
			if (m_data != nullptr)
			{
				this->deallocate(m_data);
			}
			this->m_data = b.m_data;
			this->m_size = b.m_size;
			this->m_capacity = b.m_capacity;
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		[[nodiscard]] uint8_t* data() noexcept
		{
			return m_data;
		}

		[[nodiscard]] const uint8_t* data() const noexcept
		{
			return m_data;
		}

		[[nodiscard]] size_t size() const noexcept
		{
			return m_size;
		}
		
		[[nodiscard]] size_t capacity() const noexcept
		{
			return m_capacity;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_size == 0;
		}

		[[nodiscard]] uint8_t& operator[](size_t i) noexcept
		{
			return m_data[i];
		}

		[[nodiscard]] const uint8_t& operator[](size_t i) const noexcept
		{
			return m_data[i];
		}

		[[nodiscard]] uint8_t& at(size_t i) noexcept
		{
			return m_data[i];
		}

		[[nodiscard]] const uint8_t& at(size_t i) const noexcept
		{
			return m_data[i];
		}

		[[nodiscard]] uint8_t& back() noexcept
		{
			return m_data[size() - 1];
		}

		[[nodiscard]] const uint8_t& back() const noexcept
		{
			return m_data[size() - 1];
		}

		void resize(size_t desired_size) SOUP_EXCAL
		{
			m_size = desired_size;
			if (m_capacity < desired_size)
			{
				resizeInner(desired_size);
			}
		}

	private:
		void ensureSpace(size_t desired_size) SOUP_EXCAL
		{
			SOUP_IF_UNLIKELY (m_capacity < desired_size)
			{
#if SOUP_BUFFER_NO_RESIZE
				SOUP_THROW(Exception("soup::Buffer constructed with specific size, but it did not suffice!"));
#endif
				auto new_capacity = desired_size + (desired_size >> 1); // 1.5x
				resizeInner(new_capacity);
			}
		}

		void grow(size_t amount) SOUP_EXCAL
		{
			m_size += amount;
			ensureSpace(m_size);
		}

		void resizeInner(size_t new_capacity) SOUP_EXCAL
		{
			m_data = reinterpret_cast<uint8_t*>(this->reallocate(m_data, new_capacity));
			m_capacity = new_capacity;
		}

	public:
		void reserve(size_t desired_capacity) SOUP_EXCAL
		{
			if (m_capacity < desired_capacity)
			{
				resizeInner(desired_capacity);
			}
		}

		void push_back(uint8_t elm) SOUP_EXCAL
		{
			ensureSpace(m_size + 1);
			m_data[m_size++] = elm;
		}

		void emplace_back(uint8_t elm) SOUP_EXCAL
		{
			push_back(elm);
		}

		void insert_front(size_t count, uint8_t value) SOUP_EXCAL
		{
			const auto s = m_size;
			grow(count);
			memmove(&m_data[count], &m_data[0], s);
			memset(&m_data[0], value, count);
		}

		void prepend(const void* src_data, size_t src_size) SOUP_EXCAL
		{
			const auto s = m_size;
			grow(src_size);
			memmove(&m_data[src_size], &m_data[0], s);
			memcpy(&m_data[0], src_data, src_size);
		}

		void insert_back(size_t count, uint8_t value) SOUP_EXCAL
		{
			const auto s = m_size;
			grow(count);
			memset(&m_data[s], value, count);
		}

		void append(const void* src_data, size_t src_size) SOUP_EXCAL
		{
			ensureSpace(m_size + src_size);
			memcpy(&m_data[m_size], src_data, src_size);
			m_size += src_size;
		}

		template <typename OtherAllocatorT>
		void append(const Buffer<OtherAllocatorT>& src) SOUP_EXCAL
		{
			append(src.m_data, src.m_size);
		}

		void append(const std::string& src) SOUP_EXCAL
		{
			append(src.data(), src.size());
		}

		void erase(size_t pos, size_t len) SOUP_EXCAL
		{
			memmove(&m_data[pos], &m_data[pos + len], m_size - len);
			m_size -= len;
		}

		void clear() noexcept
		{
			m_size = 0;
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL
		{
			return std::string((const char*)data(), size());
		}

		[[nodiscard]] uint8_t* release() noexcept
		{
			uint8_t* const d = m_data;
			m_data = nullptr;
			return d;
		}
	};
}
