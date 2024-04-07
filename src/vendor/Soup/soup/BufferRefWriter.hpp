#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

NAMESPACE_SOUP
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
