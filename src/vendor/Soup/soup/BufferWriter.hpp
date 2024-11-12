#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

NAMESPACE_SOUP
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
			SOUP_TRY
			{
				buf.append(data, size);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
			return true;
		}
	};
}
