#pragma once

#include <string>

namespace soup
{
	class StringBuilder : public std::string
	{
	private:
		size_t copy_start;

	public:
		using std::string::string;

		void beginCopy(const std::string& str, std::string::const_iterator it) noexcept
		{
			copy_start = (it - str.cbegin());
		}

		void endCopy(const std::string& str, std::string::const_iterator it)
		{
			append(str.data() + copy_start, it - str.cbegin() - copy_start);
		}
	};
}
