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

		void write(const char* data, size_t size) final
		{
			this->data.append(data, size);
		}
	};
}
