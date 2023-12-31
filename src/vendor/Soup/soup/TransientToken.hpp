#pragma once

#include "SharedPtr.hpp"

namespace soup
{
	class TransientTokenBase
	{
	private:
		SharedPtr<bool> sp;

	public:
		TransientTokenBase() SOUP_EXCAL;
		TransientTokenBase(bool valid) SOUP_EXCAL;

		[[nodiscard]] bool isValid() const noexcept
		{
			return *sp;
		}

		void invalidate() const noexcept;

		void reset() noexcept;

		void refresh() noexcept;
	};

	struct TransientToken : public TransientTokenBase
	{
		using TransientTokenBase::TransientTokenBase;

		~TransientToken()
		{
			invalidate();
		}
	};

	using TransientTokenRef = TransientTokenBase;
}
