#pragma once

#include "SharedPtr.hpp"

NAMESPACE_SOUP
{
	class TransientTokenBase
	{
	private:
		SharedPtr<bool> sp;

	public:
		TransientTokenBase() SOUP_EXCAL
			: sp(soup::make_shared<bool>(true))
		{
		}

		TransientTokenBase(bool valid) SOUP_EXCAL
			: sp(soup::make_shared<bool>(valid))
		{
		}

		[[nodiscard]] bool isValid() const noexcept
		{
			return *sp;
		}

		void invalidate() const noexcept
		{
			*sp = false;
		}

		void reset() noexcept
		{
			sp.reset();
		}

		void refresh() SOUP_EXCAL
		{
			invalidate();
			sp = soup::make_shared<bool>(true);
		}
	};

	struct TransientToken : public TransientTokenBase
	{
		using TransientTokenBase::TransientTokenBase;

		~TransientToken() noexcept
		{
			invalidate();
		}
	};

	using TransientTokenRef = TransientTokenBase;
}
