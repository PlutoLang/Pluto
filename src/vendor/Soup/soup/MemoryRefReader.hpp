#pragma once

#include "Reader.hpp"

#include <cstring> // memcpy

#include "type_traits.hpp"

NAMESPACE_SOUP
{
	class MemoryRefReader final : public Reader
	{
	public:
		const uint8_t* data;
		size_t size;
		size_t offset = 0;

		MemoryRefReader(const void* _data, size_t size, bool little_endian = true)
			: Reader(little_endian), data(reinterpret_cast<const uint8_t*>(_data)), size(size)
		{
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<T>)>
		MemoryRefReader(const T& t, bool little_endian = true)
			: MemoryRefReader(t.data(), t.size(), little_endian)
		{
		}

		~MemoryRefReader() final = default;

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
