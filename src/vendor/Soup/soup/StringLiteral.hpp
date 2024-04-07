#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	template <size_t Size>
	struct StringLiteral
	{
		char m_data[Size];

		SOUP_CONSTEVAL StringLiteral(const char(&in)[Size])
		{
			for (size_t i = 0; i != Size; ++i)
			{
				m_data[i] = in[i];
			}
		}

		[[nodiscard]] static constexpr size_t size() noexcept
		{
			return Size - 1;
		}

		[[nodiscard]] static constexpr size_t length() noexcept
		{
			return Size - 1;
		}

		[[nodiscard]] constexpr const char* c_str() const noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] constexpr const char* data() const noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] constexpr const char* begin() const noexcept
		{
			return &m_data[0];
		}

		[[nodiscard]] constexpr const char* end() const noexcept
		{
			return &m_data[size()];
		}
	};
}
