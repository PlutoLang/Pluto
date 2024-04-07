#pragma once

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class OstreamWriter final : public Writer
	{
	public:
		std::ostream& os;

		OstreamWriter(std::ostream& os, Endian endian = ENDIAN_LITTLE)
			: Writer(endian), os(os)
		{
		}

		~OstreamWriter() final = default;

		bool raw(void* data, size_t size) noexcept final
		{
#if SOUP_EXCEPTIONS
			try
#endif
			{
				os.write(reinterpret_cast<char*>(data), size);
			}
#if SOUP_EXCEPTIONS
			catch (...)
			{
				return false;
			}
#endif
			return true;
		}
	};
}
