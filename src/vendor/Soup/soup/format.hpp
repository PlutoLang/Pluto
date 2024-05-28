#pragma once

#include <sstream>
#include <string>

#include "base.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	// For format strings known at compile-time, std::format is faster.
	// Although the way std::format deals with custom types requires more boilerplate and ends up making it lose its time advantage.
	// In any case, if performance matters to you, do your own benchmarks and consider not using a format function at all.
	// Otherwise, use whichever makes you more productive.

	template <typename In>
	std::string format_toString(const char v)
	{
		return std::string(1, v);
	}
	
	template <typename In>
	std::string format_toString(const char* v)
	{
		return v;
	}

	template <typename In, SOUP_RESTRICT(std::is_arithmetic_v<In>)>
	std::string format_toString(const In& v)
	{
		return std::to_string(v);
	}

	template <typename In, SOUP_RESTRICT(std::is_void_v<std::remove_pointer_t<In>>)>
	std::string format_toString(const void* v)
	{
		std::stringstream stream;
		stream << v;
		return stream.str();
	}

	template <typename In, SOUP_RESTRICT(!std::is_arithmetic_v<In> && !std::is_void_v<std::remove_pointer_t<In>>)>
	std::string format_toString(const In& v)
	{
		return v.toString();
	}

	inline void format_expandLiteralPart(std::string& res, size_t& sep, const std::string& str)
	{
		sep = str.find('}', sep);
		if (sep != std::string::npos)
		{
			sep += 1;
			auto next = str.find('{', sep);
			if (next == std::string::npos)
			{
				res.append(str.substr(sep));
			}
			else
			{
				res.append(str.substr(sep, next - sep));
			}
			sep = next;
		}
	}

	template <typename T>
	void format_expandArg(std::string& res, size_t& sep, const std::string& str, const T& arg)
	{
		if (sep == std::string::npos) // More arguments than instances of "{}"?
		{
			return;
		}

		if constexpr (std::is_same_v<T, std::string>)
		{
			res.append(arg);
		}
		else
		{
			res.append(format_toString<T>(arg));
		}

		format_expandLiteralPart(res, sep, str);
	}

	template <typename...Args>
	std::string format(const std::string& str, const Args&... args)
	{
		std::string res;
		size_t sep = str.find('{');
		res.append(str.substr(0, sep));
		(format_expandArg(res, sep, str, args), ...);

		while (sep != std::string::npos) // More instances of "{}" than arguments?
		{
			format_expandLiteralPart(res, sep, str);
		}

		return res;
	}
}
