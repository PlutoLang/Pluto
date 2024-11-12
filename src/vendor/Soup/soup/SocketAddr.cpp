#include "SocketAddr.hpp"

#include "string.hpp"

NAMESPACE_SOUP
{
	bool SocketAddr::fromString(const std::string & str) SOUP_EXCAL
	{
		const size_t sep = str.find_last_of(':');
		SOUP_RETHROW_FALSE(sep != std::string::npos);
		SOUP_IF_UNLIKELY (!ip.fromString(str.substr(0, sep)))
		{
			ip.reset();
		}
		const auto opt = string::toIntOpt<uint16_t>(str.substr(sep + 1), string::TI_FULL);
		SOUP_RETHROW_FALSE(opt.has_value());
		port = Endianness::toNetwork(*opt);
		return true;
	}
}
