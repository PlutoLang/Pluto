#pragma once

#include "Reader.hpp"

#include <istream>

NAMESPACE_SOUP
{
	class IstreamReader final : public Reader
	{
	public:
		std::istream& is;

		IstreamReader(std::istream& is)
			: Reader(), is(is)
		{
		}

		~IstreamReader() final = default;

		bool hasMore() noexcept final
		{
			SOUP_TRY
			{
				return is.peek() != EOF;
			}
			SOUP_CATCH_ANY
			{
			}
			return false;
		}

		bool raw(void* data, size_t len) noexcept final
		{
			SOUP_TRY
			{
				is.read(reinterpret_cast<char*>(data), len);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
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
