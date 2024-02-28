#pragma once

#include <atomic>

#include "Capture.hpp"
#include "SelfDeletingThread.hpp"

namespace soup
{
	class PromiseBase
	{
	protected:
		Capture res{};

	public:
		PromiseBase() = default;

	protected:
		PromiseBase(Capture&& res)
			: res(std::move(res))
		{
		}

	public:
		operator bool() const noexcept
		{
			return isFulfilled();
		}

		[[nodiscard]] bool isPending() const noexcept
		{
			return res.empty();
		}

		[[nodiscard]] bool isFulfilled() const noexcept
		{
			return !isPending();
		}

		void awaitFulfilment();

		void reset() noexcept
		{
			res.reset();
		}

	protected:
		void fulfil(Capture&& res) noexcept
		{
			this->res = std::move(res);
		}
	};

	template <typename T = void>
	class Promise : public PromiseBase
	{
	public:
		Promise() = default;

		// Creates a fulfilled promise
		Promise(T&& res)
			: PromiseBase(std::move(res))
		{
		}

		Promise(T(*f)(Capture&&), Capture&& cap = {})
			: Promise()
		{
			fulfilOffThread(f, std::move(cap));
		}

		void fulfil(T&& res) SOUP_EXCAL
		{
			PromiseBase::fulfil(std::move(res));
		}

		[[nodiscard]] T& getResult() const noexcept
		{
			return res.get<T>();
		}

		void fulfilOffThread(T(*f)(Capture&&), Capture&& cap = {})
		{
			new SelfDeletingThread([](Capture&& _cap)
			{
				auto& cap = _cap.get<CaptureFulfillOffThread>();
				cap.promise.fulfil(cap.f(std::move(cap.cap)));
			}, CaptureFulfillOffThread{ *this, f, std::move(cap) });
		}

	protected:
		struct CaptureFulfillOffThread
		{
			Promise& promise;
			T(*f)(Capture&&);
			Capture cap;
		};
	};

	template <>
	class Promise<void>
	{
	private:
		std::atomic_bool fulfiled = false;

	public:
		Promise() = default;

		operator bool() const noexcept
		{
			return isFulfilled();
		}

		[[nodiscard]] bool isPending() const noexcept
		{
			return !isFulfilled();
		}

		[[nodiscard]] bool isFulfilled() const noexcept
		{
			return fulfiled;
		}

		void awaitFulfilment();

		void fulfil() noexcept
		{
			fulfiled = true;
		}

		void reset() noexcept
		{
			fulfiled = false;
		}

		void fulfilOffThread(void(*f)(Capture&&), Capture&& cap = {})
		{
			new SelfDeletingThread([](Capture&& _cap)
			{
				auto& cap = _cap.get<CaptureFulfillOffThread>();
				cap.f(std::move(cap.cap));
				cap.promise.fulfil();
			}, CaptureFulfillOffThread{ *this, f, std::move(cap) });
		}

	protected:
		struct CaptureFulfillOffThread
		{
			Promise& promise;
			void(*f)(Capture&&);
			Capture cap;
		};
	};
}
