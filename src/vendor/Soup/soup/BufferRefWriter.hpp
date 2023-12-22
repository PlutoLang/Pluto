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

		void write(const char* data, size_t size) final
		{
			buf.append(data, size);
		}
	};
}
