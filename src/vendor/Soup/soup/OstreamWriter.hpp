#pragma once

#include "Writer.hpp"

namespace soup
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

		void write(const char* data, size_t size) final
		{
			os.write(data, size);
		}
	};
}
