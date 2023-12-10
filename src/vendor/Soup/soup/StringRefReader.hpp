#pragma once

#include "ioSeekableReader.hpp"

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

		bool u8(uint8_t& v) final
		{
			if (offset == data.size())
			{
				return false;
			}
			v = (uint8_t)data.at(offset++);
			return true;
		}

	protected:
		bool str_impl(std::string& v, size_t len) final
		{
			if ((offset + len) > data.size())
			{
				return false;
			}
			v = data.substr(offset, len);
			offset += len;
			return true;
		}

	public:
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
