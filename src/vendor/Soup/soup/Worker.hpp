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
		WORKER_TYPE_USER,
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

		enum SchedulingDisposition : int
		{
			HIGH_FREQUENCY = (1 << 1) | (1 << 0),
			NEUTRAL = 1 << 0,
			LOW_FREQUENCY = 0,
		};

		uint8_t type;
		uint8_t recursions = 0;
		HoldupType holdup_type = NONE;
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
