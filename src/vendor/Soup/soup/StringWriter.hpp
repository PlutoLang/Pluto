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
#if SOUP_EXCEPTIONS
			try
#endif
			{
				this->data.append(reinterpret_cast<char*>(data), size);
			}
#if SOUP_EXCEPTIONS
			catch (...)
			{
				return false;
			}
#endif
			return true;
		}
	};
}
