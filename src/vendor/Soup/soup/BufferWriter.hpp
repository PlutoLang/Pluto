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

		bool raw(void* data, size_t size) noexcept final
		{
#if SOUP_EXCEPTIONS
			try
#endif
			{
				buf.append(data, size);
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
