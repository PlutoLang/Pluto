#pragma once

#include "Reader.hpp"

namespace soup
{
	class ioSeekableReader : public Reader
	{
	public:
		using Reader::Reader;

		bool skip(size_t len)
		{
			seek(getPosition() + len);
			return true;
		}

		[[nodiscard]] virtual size_t getPosition() = 0;

		virtual void seek(size_t pos) = 0;

		void seekBegin()
		{
			seek(0);
		}

		virtual void seekEnd() = 0;

		[[nodiscard]] size_t getRemainingBytes()
		{
			const size_t pos = getPosition();
			seekEnd();
			const size_t remaining = (getPosition() - pos);
			seek(pos);
			return remaining;
		}
	};
}
