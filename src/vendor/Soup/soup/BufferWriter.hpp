#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

namespace soup
{
	class BufferWriter final : public Writer
	{
	public:
		Buffer buf{};

		BufferWriter(Endian endian = ENDIAN_LITTLE)
			: Writer(endian)
		{
		}

		BufferWriter(bool little_endian)
			: Writer(little_endian)
		{
		}

		~BufferWriter() final = default;

		bool raw(void* data, size_t size) SOUP_EXCAL final
		{
			buf.append(data, size);
			return true;
		}
	};
}
