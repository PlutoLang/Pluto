#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

namespace soup
{
	class BufferRefWriter final : public Writer
	{
	public:
		Buffer& buf;

		BufferRefWriter(Buffer& buf, Endian endian = ENDIAN_LITTLE)
			: Writer(endian), buf(buf)
		{
		}

		~BufferRefWriter() final = default;

		bool raw(void* data, size_t size) SOUP_EXCAL final
		{
			buf.append(data, size);
			return true;
		}
	};
}
