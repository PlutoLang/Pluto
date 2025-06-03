#pragma once

#include "Worker.hpp"

NAMESPACE_SOUP
{
	class Task : public Worker
	{
	public:
		explicit Task() noexcept;
	protected:
		virtual void onTick() = 0;

	public:
		void tick();

		// For use within another Task's onTick. Returns true once done.
		[[nodiscard]] bool tickUntilDone();
	
		// To use a Task without a Scheduler
		void run();

	protected:
		// Tasks only ever have the "IDLE" holdup with the onTick-invoking callback.
		// However, that callback does not need its capture, so we can use it for arbitrary data.
		[[nodiscard]] Capture& taskCapture() noexcept { return holdup_callback.cap; }
		[[nodiscard]] const Capture& taskCapture() const noexcept { return holdup_callback.cap; }

	public:
		enum SchedulingDisposition : int
		{
			HIGH_FREQUENCY = (1 << 1) | (1 << 0),
			NEUTRAL = 1 << 0,
			LOW_FREQUENCY = 0,
		};

		[[nodiscard]] virtual int getSchedulingDisposition() const noexcept
		{
			return NEUTRAL;
		}

		[[nodiscard]] virtual std::string toString() const SOUP_EXCAL;
	};

	template <typename T>
	struct PromiseTask : public Task
	{
		T result;

		using Task::Task;

		[[nodiscard]] T&& await()
		{
			run();
			return std::move(result);
		}

		void fulfil(T&& res) noexcept
		{
			result = std::move(res);
			setWorkDone();
		}
	};
}
