#pragma once

#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct logSink
	{
		virtual ~logSink() = default;

		void writeLine(std::string message)
		{
			message.push_back('\n');
			write(std::move(message));
		}

		virtual void write(std::string&& message) = 0;
	};
}
