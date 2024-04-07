#pragma once

#include "logSink.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	extern UniquePtr<logSink> g_logSink;

	inline void logWriteLine(std::string message)
	{
		g_logSink->writeLine(std::move(message));
	}

	inline void logWrite(std::string message)
	{
		g_logSink->write(std::move(message));
	}

	inline void logSetSink(UniquePtr<logSink>&& sink)
	{
		g_logSink = std::move(sink);
	}
}
