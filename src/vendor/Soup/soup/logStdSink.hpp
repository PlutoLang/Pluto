#pragma once

#include "logSink.hpp"

#include <iostream>

NAMESPACE_SOUP
{
	struct logStdSink : public logSink
	{
		void write(std::string&& message) final
		{
			std::cout << message;
		}
	};
}
