#pragma once

#include <functional>

#include "Capture.hpp"

NAMESPACE_SOUP
{
	template <typename CaptureArgT, typename Ret, typename...Args>
	struct CallbackBase
	{
		using FuncT = Ret(Args...);
		using FuncWithCaptureT = Ret(Args..., CaptureArgT);

		FuncWithCaptureT* fp;
		Capture cap;

		CallbackBase() noexcept
			: fp(nullptr)
		{
		}

		CallbackBase(const CallbackBase&) = delete;

		CallbackBase(CallbackBase&& b) noexcept
			: fp(b.fp), cap(std::move(b.cap))
		{
		}

		CallbackBase(FuncT* fp) noexcept
			: fp(reinterpret_cast<FuncWithCaptureT*>(fp))
		{
		}

		CallbackBase(FuncWithCaptureT* fp) noexcept
			: fp(fp)
		{
		}

		CallbackBase(FuncWithCaptureT* fp, Capture&& cap) noexcept
			: fp(fp), cap(std::move(cap))
		{
		}

		static void redirect_to_std_function(Args... args, CaptureArgT cap)
		{
			return cap.template get<std::function<Ret(Args...)>>()(std::forward<Args>(args)...);
		}

		template <typename T, SOUP_RESTRICT(std::is_same_v<std::function<Ret(Args...)>, T>)>
		CallbackBase(T&& func) noexcept
			: CallbackBase(&redirect_to_std_function, std::move(func))
		{
		}

		void set(FuncWithCaptureT* fp, Capture&& cap = {}) noexcept
		{
			this->fp = fp;
			this->cap = std::move(cap);
		}

		void operator=(FuncWithCaptureT* fp) noexcept
		{
			this->fp = fp;
			this->cap.reset();
		}

		void operator=(CallbackBase&& b) noexcept
		{
			fp = b.fp;
			cap = std::move(b.cap);
		}

		void operator=(const CallbackBase& b) noexcept = delete;

		void operator=(FuncT* fp) noexcept
		{
			this->fp = reinterpret_cast<FuncWithCaptureT*>(fp);
			this->cap.reset();
		}

		template <typename T, SOUP_RESTRICT(std::is_same_v<std::function<Ret(Args...)>, T>)>
		void operator=(T&& func) noexcept
		{
			fp = &redirect_to_std_function;
			cap = std::move(func);
		}

		[[nodiscard]] constexpr operator bool() const noexcept
		{
			return fp != nullptr;
		}

		void reset()
		{
			fp = nullptr;
			cap.reset();
		}
	};

	template <typename Func>
	struct Callback;

	template <typename Ret, typename...Args>
	struct Callback<Ret(Args...)> : public CallbackBase<Capture&&, Ret, Args...>
	{
		using Base = CallbackBase<Capture&&, Ret, Args...>;

		using Base::Base;

		Ret operator() (Args&&...args)
		{
			return Base::fp(std::forward<Args>(args)..., std::move(Base::cap));
		}
	};

	template <typename Ret, typename...Args>
	struct Callback<Ret(Args...) noexcept>
	{
		using FuncT = Ret(Args...) noexcept;
		using FuncWithCaptureT = Ret(Args..., Capture&&) noexcept;

		FuncWithCaptureT* fp;
		Capture cap;

		Callback() noexcept
			: fp(nullptr)
		{
		}

		Callback(const Callback&) = delete;

		Callback(Callback&& b) noexcept
			: fp(b.fp), cap(std::move(b.cap))
		{
		}

		Callback(FuncT* fp) noexcept
			: fp(reinterpret_cast<FuncWithCaptureT*>(fp))
		{
		}

		Callback(FuncWithCaptureT* fp) noexcept
			: fp(fp)
		{
		}

		Callback(FuncWithCaptureT* fp, Capture&& cap) noexcept
			: fp(fp), cap(std::move(cap))
		{
		}

		void set(FuncWithCaptureT* fp, Capture&& cap = {}) noexcept
		{
			this->fp = fp;
			this->cap = std::move(cap);
		}

		void operator=(FuncWithCaptureT* fp) noexcept
		{
			this->fp = fp;
			this->cap.reset();
		}

		void operator=(Callback&& b) noexcept
		{
			fp = b.fp;
			cap = std::move(b.cap);
		}

		void operator=(const Callback& b) noexcept = delete;

		void operator=(FuncT* fp) noexcept
		{
			this->fp = reinterpret_cast<FuncWithCaptureT*>(fp);
			this->cap.reset();
		}

		[[nodiscard]] constexpr operator bool() const noexcept
		{
			return fp != nullptr;
		}

		void reset()
		{
			fp = nullptr;
			cap.reset();
		}

		Ret operator() (Args&&...args)
		{
			return fp(std::forward<Args>(args)..., std::move(cap));
		}
	};

	template <typename Func>
	struct EventHandler;

	template <typename Ret, typename...Args>
	struct EventHandler<Ret(Args...)> : public CallbackBase<const Capture&, Ret, Args...>
	{
		using Base = CallbackBase<const Capture&, Ret, Args...>;

		using Base::Base;

		Ret operator() (Args&&...args) const
		{
			return Base::fp(std::forward<Args>(args)..., Base::cap);
		}
	};
}
