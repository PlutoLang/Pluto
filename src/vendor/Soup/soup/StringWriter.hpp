#pragma once

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class StringWriter final : public Writer
	{
	public:
		std::string data{};

		StringWriter(Endian endian = ENDIAN_LITTLE)
			: Writer(endian)
		{
		}

		StringWriter(bool little_endian)
			: Writer(little_endian)
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
	};
}
