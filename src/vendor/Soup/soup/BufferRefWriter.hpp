#pragma once

#include "Writer.hpp"

#include "Buffer.hpp"

NAMESPACE_SOUP
{
	class BufferRefWriter final : public Writer
	{
	public:
		Buffer<>& buf;

		BufferRefWriter(Buffer<>& buf)
			: Writer(), buf(buf)
		{
		}

		~BufferRefWriter() final = default;

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
