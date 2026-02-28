#pragma once

#include "SharedPtr.hpp"

NAMESPACE_SOUP
{
	class TransientTokenBase
	{
	public:
		SharedPtr<bool> sp;

		explicit TransientTokenBase(bool valid) SOUP_EXCAL
			: TransientTokenBase(soup::make_shared<bool>(valid))
		{
		}

	protected:
		TransientTokenBase(SharedPtr<bool>&& sp) noexcept
			: sp(std::move(sp))
		{
		}

		TransientTokenBase(const SharedPtr<bool>& sp) noexcept
			: sp(sp)
		{
		}

	public:
		[[nodiscard]] bool isValid() const noexcept
		{
			return *sp;
		}

		void reset() SOUP_EXCAL
		{
			sp = soup::make_shared<bool>(false);
		}
	};

	struct TransientToken : public TransientTokenBase
	{
		using TransientTokenBase::TransientTokenBase;

		TransientToken() SOUP_EXCAL
			: TransientTokenBase(soup::make_shared<bool>(true))
		{
		}

		TransientToken(const TransientToken& tt) = delete;

		TransientToken(TransientToken&& tt) SOUP_EXCAL
			: TransientTokenBase(std::move(tt.sp))
		{
			tt.reset();
		}
		
		void operator=(const TransientToken& tt) = delete;

		void operator=(TransientToken&& tt) SOUP_EXCAL
		{
			sp = std::move(tt.sp);
			tt.reset();
		}

		~TransientToken() noexcept
		{
			invalidate();
		}

		void invalidate() const noexcept
		{
			*sp = false;
		}

		void refresh() SOUP_EXCAL
		{
			invalidate();
			sp = soup::make_shared<bool>(true);
		}
	};

	struct TransientTokenRef : public TransientTokenBase
	{
		using TransientTokenBase::TransientTokenBase;

		TransientTokenRef() SOUP_EXCAL
			: TransientTokenBase(soup::make_shared<bool>(false))
		{
		}

		TransientTokenRef(const TransientTokenBase& tt) noexcept
			: TransientTokenBase(tt.sp)
		{
		}
	};
}
