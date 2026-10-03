#include "log.hpp"

#include <iostream>

NAMESPACE_SOUP
{
	void log_write_std(std::string&& msg)
	{
		std::cout << msg;
	}
}
