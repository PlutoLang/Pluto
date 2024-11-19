#pragma once

#include <cstring> // memcpy

#include "alloc.hpp"
#include "base.hpp"
#ifdef _DEBUG
#include "Exception.hpp"
#endif

NAMESPACE_SOUP
{
	template <typename T>
	class IntVector
	{
	public:
		T* m_data = nullptr;
		size_t m_capacity = 0;
		size_t m_size = 0;

		explicit constexpr IntVector() noexcept = default;

		explicit IntVector(const IntVector<T>& b) SOUP_EXCAL
			: m_capacity(b.m_capacity), m_size(b.m_size)
		{
			if (m_capacity != 0)
			{
				m_data = (T*)soup::malloc(m_capacity * sizeof(T));
				memcpy(m_data, b.m_data, m_size * sizeof(T));
			}
		}

		explicit IntVector(IntVector<T>&& b) noexcept
			: m_data(b.m_data), m_capacity(b.m_capacity), m_size(b.m_size)
		{
			b.m_data = nullptr;
			b.m_capacity = 0;
			b.m_size = 0;
		}

		~IntVector() noexcept
		{
			free();
		}

		void operator=(const IntVector<T>& b) SOUP_EXCAL
		{
			if (m_capacity < b.m_size)
			{
				free();
				m_capacity = b.m_size;
				m_data = (T*)soup::malloc(m_capacity * sizeof(T));
			}

			m_size = b.m_size;
			memcpy(m_data, b.m_data, m_size * sizeof(T));
		}

		void operator=(IntVector<T>&& b) noexcept
		{
			free();

			m_data = b.m_data;
			m_size = b.m_size;
			m_capacity = b.m_capacity;

			b.m_data = nullptr;
			b.m_capacity = 0;
			b.m_size = 0;
		}

		[[nodiscard]] constexpr size_t size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] constexpr size_t capacity() const noexcept
		{
			return m_capacity;
		}

		[[nodiscard]] SOUP_FORCEINLINE T& operator[](size_t idx) noexcept
		{
			return m_data[idx];
		}

		[[nodiscard]] SOUP_FORCEINLINE const T& operator[](size_t idx) const noexcept
		{
			return m_data[idx];
		}

		[[nodiscard]] T& at(size_t idx)
#ifndef _DEBUG
			noexcept
#endif
		{
#ifdef _DEBUG
			SOUP_IF_UNLIKELY (idx >= size())
			{
				SOUP_THROW(Exception("Out of range"));
			}
#endif
			return m_data[idx];
		}

		[[nodiscard]] const T& at(size_t idx) const
#ifndef _DEBUG
			noexcept
#endif
		{
#ifdef _DEBUG
			SOUP_IF_UNLIKELY (idx >= size())
			{
				SOUP_THROW(Exception("Out of range"));
			}
#endif
			return m_data[idx];
		}

		void emplaceZeroesFront(size_t elms) noexcept
		{
#if true
			SOUP_IF_UNLIKELY (m_size + elms > m_capacity)
			{
				m_capacity = m_size + elms;
				auto data = reinterpret_cast<T*>(soup::malloc(m_capacity * sizeof(T)));
				memcpy(&data[elms], &m_data[0], m_size * sizeof(T));
				memset(&data[0], 0, elms * sizeof(T));
				m_data = data;
				m_size = m_capacity;
				return;
			}
#else
			while (m_size + elms > m_capacity)
			{
				makeSpaceForMoreElements();
			}
#endif
			memmove(&m_data[elms], &m_data[0], m_size * sizeof(T));
			memset(&m_data[0], 0, elms * sizeof(T));
			m_size += elms;
		}

		void emplace_back(T val) SOUP_EXCAL
		{
			SOUP_IF_UNLIKELY (m_size == m_capacity)
			{
				makeSpaceForMoreElements();
			}
			m_data[m_size] = val;
			++m_size;
		}

		void erase(size_t first, size_t last) noexcept
		{
			const auto count = (last - first);
			m_size -= count;
			if (m_size != first)
			{
				memcpy(&m_data[first], &m_data[last], (m_size - first) * sizeof(T));
			}
		}

		void eraseFirst(size_t num) noexcept
		{
			m_size -= num;
			memcpy(&m_data[0], &m_data[num], m_size * sizeof(T));
		}

		void preallocate() SOUP_EXCAL
		{
			if (m_capacity == 0)
			{
				m_capacity = (0x1000 / sizeof(T));
				m_data = reinterpret_cast<T*>(soup::malloc(m_capacity * sizeof(T)));
			}
		}

	protected:
		void makeSpaceForMoreElements() SOUP_EXCAL
		{
			m_capacity += (0x1000 / sizeof(T));
			m_data = reinterpret_cast<T*>(soup::realloc(m_data, m_capacity * sizeof(T)));
		}

	public:
		void clear() noexcept
		{
			m_size = 0;
		}

		/*void reset() noexcept
		{
			clear();
			if (m_capacity != 0)
			{
				m_capacity = 0;
				soup::free(m_data);
				m_data = nullptr;
			}
		}*/

	protected:
		void free() noexcept
		{
			if (m_capacity != 0)
			{
				soup::free(m_data);
			}
		}
	};
}
