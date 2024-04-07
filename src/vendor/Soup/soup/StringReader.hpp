#pragma once

#include "Reader.hpp"

#include <cstring> // memcpy

NAMESPACE_SOUP
{
	class StringReader final : public Reader
	{
	public:
		std::string data;
		size_t offset = 0;

		StringReader(Endian endian = ENDIAN_LITTLE) noexcept
			: Reader(endian)
		{
		}

		StringReader(std::string&& data, Endian endian = ENDIAN_LITTLE) noexcept
			: Reader(endian), data(std::move(data))
		{
		}

		StringReader(std::string&& data, bool little_endian) noexcept
			: Reader(little_endian), data(std::move(data))
		{
		}

		~StringReader() noexcept final = default;

		void operator =(std::string&& new_data) noexcept
		{
			data = std::move(new_data);
			offset = 0;
		}

		bool hasMore() noexcept final
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
			offset = data.size();
		}

		// Faster alternative to std::stringstream + std::getline
		bool getLine(std::string& line) SOUP_EXCAL final
		{
			size_t next = data.find('\n', offset);
			SOUP_IF_LIKELY (next != std::string::npos)
			{
				line = data.substr(offset, next - offset);
				offset = next + 1;
				return true;
			}
			// We're on the last line
			if (offset != data.size()) // Not the last byte?
			{
				line = data.substr(offset);
				offset = data.size();
				return true;
			}
			return false;
		}
	};
}
