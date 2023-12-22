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

		void write(const char* data, size_t size) final
		{
			buf.append(data, size);
		}
	};
}
