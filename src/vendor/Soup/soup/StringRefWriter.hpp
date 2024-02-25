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

		bool raw(void* data, size_t size) SOUP_EXCAL final
		{
			str.append(reinterpret_cast<char*>(data), size);
			return true;
		}
	};
}
