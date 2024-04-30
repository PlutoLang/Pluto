#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	struct MimeType
	{
		inline static const char* TEXT_PLAIN = "text/plain;charset=utf-8";
		inline static const char* TEXT_HTML = "text/html;charset=utf-8";

		inline static const char* IMAGE_PNG = "image/png";
		inline static const char* IMAGE_SVG = "image/svg+xml";
	};
}
