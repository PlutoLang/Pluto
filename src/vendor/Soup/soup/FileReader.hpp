#pragma once

#include "Reader.hpp"

#include <filesystem>
#include <fstream>

#include "type_traits.hpp"

NAMESPACE_SOUP
{
	class FileReader final : public Reader
	{
	public:
		std::ifstream s;

		FileReader(const std::string& path)
			: Reader(), s(path, std::ios::binary)
		{
		}

#if SOUP_WINDOWS && !SOUP_CROSS_COMPILE
		FileReader(const std::wstring& path)
			: Reader(), s(path, std::ios::binary)
		{
		}
#endif

		template <class T, SOUP_RESTRICT(std::is_same_v<T, std::filesystem::path>)>
		FileReader(const T& path)
			: Reader(), s(path, std::ios::binary)
		{
		}

		~FileReader() final = default;

		bool hasMore() noexcept final
		{
			SOUP_TRY
			{
				return s.peek() != EOF;
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
				s.read(reinterpret_cast<char*>(data), len);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
			return s.rdstate() == 0;
		}

		bool getLine(std::string& line) SOUP_EXCAL final
		{
			if (Reader::getLine(line))
			{
				if (!line.empty()
					&& line.back() == '\r'
					)
				{
					line.pop_back();
				}
				return true;
			}
			return false;
		}

		[[nodiscard]] size_t getPosition() final
		{
			return static_cast<size_t>(s.tellg());
		}

		void seek(size_t pos) final
		{
			s.seekg(pos);
			s.clear();
		}

		void seekEnd() final
		{
			s.seekg(0, std::ios::end);
		}
	};
}
