#include "version_compare.hpp"

#include "string.hpp"

NAMESPACE_SOUP
{
	strong_ordering version_compare(std::string in_a, std::string in_b) SOUP_EXCAL
	{
		std::vector<long> a{};
		std::vector<long> b{};

		string::replaceAll(in_a, "-", ".");
		string::replaceAll(in_b, "-", ".");

		for (const auto& s : string::explode(in_a, '.'))
		{
			a.emplace_back(string::toInt<long>(s, -1));
		}
		for (const auto& s : string::explode(in_b, '.'))
		{
			b.emplace_back(string::toInt<long>(s, -1));
		}

		if (a.size() != b.size())
		{
			if (a.size() > b.size())
			{
				b.insert(b.end(), a.size() - b.size(), 0);
			}
			else
			{
				a.insert(a.end(), b.size() - a.size(), 0);
			}
		}

		for (size_t i = 0; i != a.size(); ++i)
		{
			auto c = SOUP_SPACESHIP(a[i], b[i]);
			if (c != 0)
			{
				return c;
			}
		}

		return strong_ordering::equal;
	}
}
