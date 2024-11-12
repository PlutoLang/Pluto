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
		const auto opt = string::toInt<uint16_t, string::TI_FULL>(str.substr(sep + 1));
		SOUP_RETHROW_FALSE(opt.has_value());
		port = Endianness::toNetwork(*opt);
		return true;
	}
}
