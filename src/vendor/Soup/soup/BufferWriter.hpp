#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

NAMESPACE_SOUP
{
	class BufferWriter final : public Writer
	{
	public:
		Buffer buf{};

		BufferWriter()
			: Writer(ENDIAN_LITTLE)
		{
		}

		[[deprecated]] BufferWriter(Endian endian)
			: Writer(endian)
		{
		}

		[[deprecated]] BufferWriter(bool little_endian)
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

		[[nodiscard]] size_t getPosition() final
		{
			return buf.size();
		}
	};
}
