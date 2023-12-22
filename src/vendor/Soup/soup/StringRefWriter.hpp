#pragma once

#include "Writer.hpp"

namespace soup
{
	class StringRefWriter final : public Writer
	{
	public:
		std::string& str;

		StringRefWriter(std::string& str, bool little_endian = true)
			: Writer(little_endian), str(str)
		{
		}

		~StringRefWriter() final = default;

		void write(const char* data, size_t size) final
		{
			str.append(data, size);
		}
	};
}
