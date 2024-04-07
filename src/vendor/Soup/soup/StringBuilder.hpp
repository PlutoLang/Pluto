#pragma once

#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	class StringBuilder : public std::string
	{
	private:
		const char* copy_start;

	public:
		using std::string::string;

		void beginCopy(const char* i) noexcept
		{
			copy_start = i;
		}

		void endCopy(const char* i)
		{
			append(copy_start, i);
		}

		void beginCopy(const std::string& str, std::string::const_iterator it) noexcept
		{
			return beginCopy(str.data() + (it - str.cbegin()));
		}

		void endCopy(const std::string& str, std::string::const_iterator it)
		{
			return endCopy(str.data() + (it - str.cbegin()));
		}
	};
}
