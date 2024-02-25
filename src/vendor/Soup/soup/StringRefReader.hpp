#pragma once

#include "ioSeekableReader.hpp"

#include <cstring> // memcpy

namespace soup
{
	class StringRefReader final : public ioSeekableReader
	{
	public:
		const std::string& data;
		size_t offset = 0;

		StringRefReader(const std::string& data, bool little_endian = true)
			: ioSeekableReader(little_endian), data(data)
		{
		}

		~StringRefReader() final = default;

		bool hasMore() final
		{
			return offset != data.size();
		}

		bool raw(void* data, size_t len) noexcept final
		{
			SOUP_IF_UNLIKELY ((offset + len) > this->data.size())
			{
				return false;
			}
			memcpy(reinterpret_cast<char*>(data), this->data.data() + offset, len);
			offset += len;
			return true;
		}

		[[nodiscard]] size_t getPosition() final
		{
			return offset;
		}

		void seek(size_t pos) final
		{
			offset = pos;
		}

		void seekEnd() final
		{
			offset = data.size();
		}
	};
}
