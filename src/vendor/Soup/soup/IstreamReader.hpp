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

		bool hasMore() noexcept final
		{
#if SOUP_EXCEPTIONS
			try
#endif
			{
				return is.peek() != EOF;
			}
#if SOUP_EXCEPTIONS
			catch (...)
			{
			}
			return false;
#endif
		}

		bool raw(void* data, size_t len) noexcept final
		{
#if SOUP_EXCEPTIONS
			try
#endif
			{
				is.read(reinterpret_cast<char*>(data), len);
			}
#if SOUP_EXCEPTIONS
			catch (...)
			{
				return false;
			}
#endif
			return is.rdstate() == 0;
		}

		bool getLine(std::string& line) SOUP_EXCAL final
		{
			std::getline(is, line);
			return is.operator bool();
		}

		[[nodiscard]] size_t getPosition() final
		{
			return static_cast<size_t>(is.tellg());
		}

		void seek(size_t pos) final
		{
			is.seekg(pos);
			is.clear();
		}

		void seekEnd() final
		{
			is.seekg(0, std::ios::end);
		}
	};
}
