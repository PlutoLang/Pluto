#pragma once

#include "Writer.hpp"

namespace soup
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

		bool raw(void* data, size_t size) SOUP_EXCAL final
		{
			this->data.append(reinterpret_cast<char*>(data), size);
			return true;
		}
	};
}
