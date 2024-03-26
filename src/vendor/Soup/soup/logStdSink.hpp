#pragma once

#include "logSink.hpp"

#include <iostream>

namespace soup
{
	struct logStdSink : public logSink
	{
		void write(std::string&& message) final
		{
			std::cout << message;
		}
	};
}
