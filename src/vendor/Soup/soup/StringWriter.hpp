#pragma once

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class StringWriter final : public Writer
	{
	public:
		std::string data{};

		StringWriter()
			: Writer()
		{
		}

		~StringWriter() final = default;

		bool raw(void* data, size_t size) noexcept final
		{
			SOUP_TRY
			{
				this->data.append(reinterpret_cast<char*>(data), size);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
			return true;
		}

		[[nodiscard]] size_t getPosition() final
		{
			return data.size();
		}
	};
}
