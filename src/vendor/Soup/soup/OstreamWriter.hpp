#pragma once

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class OstreamWriter final : public Writer
	{
	public:
		std::ostream& os;

		OstreamWriter(std::ostream& os)
			: Writer(), os(os)
		{
		}

		~OstreamWriter() final = default;

		bool raw(void* data, size_t size) noexcept final
		{
			SOUP_TRY
			{
				os.write(reinterpret_cast<char*>(data), size);
			}
			SOUP_CATCH_ANY
			{
				return false;
			}
			return true;
		}

		[[nodiscard]] size_t getPosition() final
		{
			return os.tellp();
		}
	};
}
