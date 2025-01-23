#pragma once

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class StringRefWriter final : public Writer
	{
	public:
		std::string& str;

		StringRefWriter(std::string& str)
			: Writer(), str(str)
		{
		}

		~StringRefWriter() final = default;

		bool raw(void* data, size_t size) noexcept final
		{
			SOUP_TRY
			{
				str.append(reinterpret_cast<char*>(data), size);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
			return true;
		}

		[[nodiscard]] size_t getPosition() final
		{
			return str.size();
		}
	};
}
