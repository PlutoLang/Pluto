#pragma once

#include <cstring>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	template <size_t S>
	class ShortString
	{
	private:
		char m_data[S] = { 0 };
		const char term = 0;

	public:
		ShortString() noexcept
		{
		}

		ShortString(const std::string& b) noexcept
		{
			operator=(b);
		}

		ShortString(const char* b) noexcept
		{
			operator=(b);
		}

		ShortString(const ShortString<S>& b) noexcept
		{
			operator=(b);
		}

		void operator =(const std::string& b) noexcept
		{
			operator=(b.c_str());
		}

		void operator =(const char* b) noexcept
		{
			SOUP_DEBUG_ASSERT(strlen(b) <= S);
			strncpy(data(), b, S);
		}

		void operator =(const ShortString<S>& b) noexcept
		{
			strncpy(data(), b.data(), S);
		}

		[[nodiscard]] char* data() noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] const char* data() const noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] const char* c_str() const noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] static size_t capacity() noexcept
		{
			return S;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_data[0] == 0;
		}
		
		[[nodiscard]] size_t size() const noexcept
		{
			return strlen(c_str());
		}

		[[nodiscard]] char& operator [](size_t Index) noexcept
		{
			SOUP_DEBUG_ASSERT(Index < S);
			return m_data[Index];
		}

		[[nodiscard]] const char& operator [](size_t Index) const noexcept
		{
			SOUP_DEBUG_ASSERT(Index < S);
			return m_data[Index];
		}
	};
	static_assert(sizeof(ShortString<1>) == 2);
	static_assert(sizeof(ShortString<2>) == 3);
	static_assert(sizeof(ShortString<3>) == 4);
	static_assert(sizeof(ShortString<4>) == 5);
	static_assert(sizeof(ShortString<5>) == 6);
	static_assert(sizeof(ShortString<6>) == 7);
	static_assert(sizeof(ShortString<7>) == 8);
	static_assert(sizeof(ShortString<8>) == 9);
	static_assert(sizeof(ShortString<9>) == 10);
}
