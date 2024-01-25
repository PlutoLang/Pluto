#include "filesystem.hpp"

#include "base.hpp"
#include "string.hpp"

namespace soup
{
	std::filesystem::path filesystem::u8path(const std::string& str)
	{
#if SOUP_CPP20
		return soup::string::toUtf8Type(str);
#else
		return std::filesystem::u8path(str);
#endif
	}

	bool filesystem::exists_case_sensitive(const std::filesystem::path& p)
	{
		return std::filesystem::exists(p)
#if SOUP_WINDOWS
			&& p.filename() == std::filesystem::canonical(p).filename()
#endif
			;
	}
}
