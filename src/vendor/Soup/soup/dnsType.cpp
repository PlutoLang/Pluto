#include "dnsType.hpp"

#include <cstring>

NAMESPACE_SOUP
{
	std::string dnsTypeToString(dnsType type)
	{
		switch (type)
		{
		case DNS_A: return "A";
		case DNS_AAAA: return "AAAA";
		case DNS_CNAME: return "CNAME";
		case DNS_PTR: return "PTR";
		case DNS_TXT: return "TXT";
		case DNS_MX: return "MX";
		case DNS_SRV: return "SRV";
		case DNS_NS: return "NS";
		default:;
		}
		return std::to_string(type);
	}

	dnsType dnsTypeFromString(const char* str)
	{
		if (strcmp(str, "A") == 0) return DNS_A;
		if (strcmp(str, "AAAA") == 0) return DNS_AAAA;
		if (strcmp(str, "CNAME") == 0) return DNS_CNAME;
		if (strcmp(str, "PTR") == 0) return DNS_PTR;
		if (strcmp(str, "TXT") == 0) return DNS_TXT;
		if (strcmp(str, "MX") == 0) return DNS_MX;
		if (strcmp(str, "SRV") == 0) return DNS_SRV;
		if (strcmp(str, "NS") == 0) return DNS_NS;

		return DNS_NONE;
	}
}
