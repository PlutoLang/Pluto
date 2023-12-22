#include "TransientToken.hpp"

namespace soup
{
	TransientTokenBase::TransientTokenBase()
		: sp(soup::make_shared<bool>(true))
	{
	}

	TransientTokenBase::TransientTokenBase(bool valid)
		: sp(soup::make_shared<bool>(valid))
	{
	}

	void TransientTokenBase::invalidate() const noexcept
	{
		*sp = false;
	}

	void TransientTokenBase::reset() noexcept
	{
		sp.reset();
	}

	void TransientTokenBase::refresh() noexcept
	{
		invalidate();
		sp = soup::make_shared<bool>(true);
	}
}
