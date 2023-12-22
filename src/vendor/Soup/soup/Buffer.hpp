#pragma once

#include <cstring> // memcpy
#include <string>

#ifndef SOUP_BUFFER_NO_RESIZE
#define SOUP_BUFFER_NO_RESIZE false
#endif

#if SOUP_BUFFER_NO_RESIZE
#include "Exception.hpp"
#endif

namespace soup
{
	// This extremely simple implementation outperforms std::vector & std::string, barring magic optimisations, e.g. clang with std::string::push_back.
	class Buffer
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

		Buffer(size_t capacity) noexcept
			: m_data(reinterpret_cast<uint8_t*>(malloc(capacity))), m_capacity(capacity)
		{
#if SOUP_BUFFER_NO_RESIZE
			no_resize = true;
#endif
		}

		Buffer(const Buffer& b) noexcept
			: Buffer(b.m_size)
		{
			append(b);
		}

		Buffer(Buffer&& b) noexcept
			: m_data(b.m_data), m_size(b.m_size), m_capacity(b.m_capacity)
		{
			b.m_data = nullptr;
			b.m_size = 0;
			b.m_capacity = 0;
		}

		~Buffer() noexcept
		{
			reset();
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

		void resize(size_t desired_size) noexcept
		{
			m_size = desired_size;
			if (m_capacity < desired_size)
			{
				resizeInner(desired_size);
			}
		}

	private:
		void ensureSpace(size_t desired_size) noexcept
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

		void grow(size_t amount) noexcept
		{
			m_size += amount;
			ensureSpace(m_size);
		}

		void resizeInner(size_t new_capacity) noexcept
		{
			m_data = reinterpret_cast<uint8_t*>(m_data ? realloc(m_data, new_capacity) : malloc(new_capacity));
			m_capacity = new_capacity;
		}

	public:
		void reserve(size_t desired_capacity) noexcept
		{
			if (m_capacity < desired_capacity)
			{
				resizeInner(desired_capacity);
			}
		}

		void push_back(uint8_t elm) noexcept
		{
			ensureSpace(m_size + 1);
			m_data[m_size++] = elm;
		}

		void emplace_back(uint8_t elm) noexcept
		{
			push_back(elm);
		}

		void insert_front(size_t count, uint8_t value) noexcept
		{
			const auto s = m_size;
			grow(count);
			memcpy(&m_data[count], &m_data[0], s);
			memset(&m_data[0], value, count);
		}

		void insert_back(size_t count, uint8_t value) noexcept
		{
			const auto s = m_size;
			grow(count);
			memset(&m_data[s], value, count);
		}

		void append(const void* src_data, size_t src_size) noexcept
		{
			ensureSpace(m_size + src_size);
			memcpy(&m_data[m_size], src_data, src_size);
			m_size += src_size;
		}

		void append(const Buffer& src) noexcept
		{
			append(src.m_data, src.m_size);
		}

		void erase(size_t pos, size_t len) noexcept
		{
			memmove(&m_data[pos], &m_data[pos + len], m_size - len);
			m_size -= len;
		}

		void clear() noexcept
		{
			m_size = 0;
		}

		[[nodiscard]] std::string toString() const
		{
			return std::string((const char*)data(), size());
		}

		[[nodiscard]] uint8_t* release() noexcept
		{
			uint8_t* const d = m_data;
			m_data = nullptr;
			return d;
		}

	private:
		void reset() noexcept
		{
			if (m_data != nullptr)
			{
				free(m_data);
				m_data = nullptr;
			}
		}
	};
}
