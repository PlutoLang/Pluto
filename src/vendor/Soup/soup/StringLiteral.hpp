#pragma once

namespace soup
{
	template <size_t Size>
	struct StringLiteral
	{
		char data[Size];

		SOUP_CONSTEVAL StringLiteral(const char(&in)[Size])
		{
			for (size_t i = 0; i != Size; ++i)
			{
				data[i] = in[i];
			}
		}

		[[nodiscard]] SOUP_CONSTEVAL size_t size() const noexcept
		{
			return Size;
		}

		[[nodiscard]] SOUP_CONSTEVAL const char* c_str() const noexcept
		{
			return &data[0];
		}
	};
}
