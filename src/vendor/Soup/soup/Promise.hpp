#pragma once

#include <atomic>

#include "Capture.hpp"
#include "SelfDeletingThread.hpp"

NAMESPACE_SOUP
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
		Promise(const Promise<T>&) = delete;
		Promise(Promise<T>&&) = delete;
		void operator=(const Promise<T>&) = delete;
		void operator=(Promise<T>&&) = delete;

		// Creates a fulfilled promise
		Promise(T&& res)
			: PromiseBase(std::move(res))
		{
		}

#if !SOUP_WASM
		Promise(T(*f)(Capture&&), Capture&& cap = {})
			: Promise()
		{
			fulfilOffThread(f, std::move(cap));
		}
#endif

		void fulfil(T&& res) SOUP_EXCAL
		{
			PromiseBase::fulfil(std::move(res));
		}

		[[nodiscard]] T& getResult() const noexcept
		{
			return res.get<T>();
		}

#if !SOUP_WASM
		void fulfilOffThread(T(*f)(Capture&&), Capture&& cap = {})
		{
			new SelfDeletingThread([](Capture&& _cap)
			{
				auto& cap = _cap.get<CaptureFulfillOffThread>();
				cap.promise.fulfil(cap.f(std::move(cap.cap)));
			}, CaptureFulfillOffThread{ *this, f, std::move(cap) });
		}
#endif

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
		Promise(const Promise<void>&) = delete;
		Promise(Promise<void>&&) = delete;
		void operator=(const Promise<void>&) = delete;
		void operator=(Promise<void>&&) = delete;

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

#if !SOUP_WASM
		void fulfilOffThread(void(*f)(Capture&&), Capture&& cap = {})
		{
			SOUP_DEBUG_ASSERT(!isFulfilled());
			new SelfDeletingThread([](Capture&& _cap)
			{
				auto& cap = _cap.get<CaptureFulfillOffThread>();
				cap.f(std::move(cap.cap));
				cap.promise.fulfil();
			}, CaptureFulfillOffThread{ *this, f, std::move(cap) });
		}
#endif

	protected:
		struct CaptureFulfillOffThread
		{
			Promise& promise;
			void(*f)(Capture&&);
			Capture cap;
		};
	};
}
