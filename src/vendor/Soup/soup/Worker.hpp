#pragma once

#include "fwd.hpp"

#include <cstdint>
#include <string>
#include <utility>

#include "Callback.hpp"
#include "Capture.hpp"

NAMESPACE_SOUP
{
	enum : uint8_t
	{
		WORKER_TYPE_UNSPECIFIED = 0,
		WORKER_TYPE_SOCKET,
		WORKER_TYPE_TASK,
		WORKER_TYPE_USER // use this as the counting base for your own types
	};

	enum : uint8_t
	{
		WORKER_FLAG_USER, // use this as the counting base for your own flags
		WORKER_FLAG_SIZE = 8
	};

	struct Worker
	{
		enum HoldupType : uint8_t
		{
			NONE,
			SOCKET,
			IDLE, // call holdup callback whenever the scheduler has some idle time
			PROMISE_BASE,
			PROMISE_VOID,
		};

		uint8_t type;
		uint8_t recursions = 0;
		HoldupType holdup_type = NONE;
		uint8_t flags = 0;
		Callback<void(Worker&)> holdup_callback;
		void* holdup_data;

		Worker(uint8_t type) noexcept
			: type(type)
		{
		}

		virtual ~Worker() = default;

		Worker& operator=(Worker&& b) noexcept = default;

		void fireHoldupCallback()
		{
			recursions = 0;
			holdup_callback(*this);
		}

		void awaitPromiseCompletion(PromiseBase* p, void(*f)(Worker&, Capture&&), Capture&& cap = {});
		void awaitPromiseCompletion(Promise<void>* p, void(*f)(Worker&, Capture&&), Capture&& cap = {});

		[[nodiscard]] bool isWorkDone() const noexcept { return holdup_type == NONE; }
		void setWorkDone() noexcept { holdup_type = NONE; }

		void disallowRecursion() noexcept { recursions = 19; }
		[[nodiscard]] bool canRecurse() noexcept { return ++recursions != 20; }

		[[nodiscard]] std::string toString() const SOUP_EXCAL;
	};
}
