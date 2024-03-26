#pragma once

#include "Reader.hpp"

#include <cstring> // memcpy

namespace soup
{
	class StringRefReader final : public Reader
	{
	public:
		const char* data;
		size_t size;
		size_t offset = 0;

		StringRefReader(const std::string& str, bool little_endian = true)
			: Reader(little_endian), data(str.data()), size(str.size())
		{
		}

		StringRefReader(const char* data, size_t size, bool little_endian = true)
			: Reader(little_endian), data(data), size(size)
		{
		}

		~StringRefReader() final = default;

		bool hasMore() noexcept final
		{
			return offset != size;
		}

		bool raw(void* data, size_t len) noexcept final
		{
			SOUP_IF_UNLIKELY ((offset + len) > this->size)
			{
				return false;
			}
			memcpy(reinterpret_cast<char*>(data), this->data + offset, len);
			offset += len;
			return true;
		}

		[[nodiscard]] size_t getPosition() noexcept final
		{
			return offset;
		}

		void seek(size_t pos) noexcept final
		{
			offset = pos;
		}

		void seekEnd() noexcept final
		{
			offset = size;
		}
	};
}
