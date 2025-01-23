#include "dnsName.hpp"

#include "MemoryRefReader.hpp"

NAMESPACE_SOUP
{
	std::vector<std::string> dnsName::resolve(const std::string& data, unsigned int max_recursions) const
	{
		std::vector<std::string> res = name;
		if (ptr != 0
			&& max_recursions != 0
			)
		{
			MemoryRefReader sr(data);
			sr.offset = ptr;

			dnsName cont;
			cont.read(sr);

			auto vec = cont.resolve(data, max_recursions - 1);
			res.insert(res.end(), std::make_move_iterator(vec.begin()), std::make_move_iterator(vec.end()));
		}
		return res;
	}
}
