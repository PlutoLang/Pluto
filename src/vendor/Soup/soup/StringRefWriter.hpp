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

		bool raw(void* data, size_t size) noexcept final
		{
#if SOUP_EXCEPTIONS
			try
#endif
			{
				str.append(reinterpret_cast<char*>(data), size);
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
