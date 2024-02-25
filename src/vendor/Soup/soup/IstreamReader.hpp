#pragma once

#include "Reader.hpp"

#include <istream>

namespace soup
{
	class IstreamReader final : public Reader
	{
	public:
		std::istream& is;

		IstreamReader(std::istream& is, Endian endian = ENDIAN_LITTLE)
			: Reader(endian), is(is)
		{
		}

		~IstreamReader() final = default;

		bool hasMore() final
		{
			return is.peek() != EOF;
		}

		bool raw(void* data, size_t len) final
		{
			return !is.read(reinterpret_cast<char*>(data), len).bad();
		}

		bool getLine(std::string& line) noexcept final
		{
			std::getline(is, line);
			return is.operator bool();
		}
	};
}
