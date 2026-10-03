#pragma once

#include "base.hpp"

#include <string>

NAMESPACE_SOUP
{
	using log_write_t = void(*)(std::string&&);

	extern void log_write_std(std::string&& msg);

	inline log_write_t g_log_write = &log_write_std;

	inline void logWriteLine(std::string msg)
	{
		msg.push_back('\n');
		g_log_write(std::move(msg));
	}

	inline void logWrite(std::string msg)
	{
		g_log_write(std::move(msg));
	}

	inline void logSetWrite(log_write_t f)
	{
		g_log_write = f;
	}
}
