#include "Task.hpp"

#include "base.hpp"

#include "Scheduler.hpp"

NAMESPACE_SOUP
{
	Task::Task() noexcept
		: Worker(WORKER_TYPE_TASK)
	{
		holdup_type = Worker::IDLE;
		holdup_callback.set([](Worker& w, Capture&&)
		{
			static_cast<Task&>(w).onTick();
		});
	}

	void Task::tick()
	{
		if (!isWorkDone())
		{
			onTick();
		}
	}

	bool Task::tickUntilDone()
	{
		SOUP_IF_UNLIKELY (isWorkDone())
		{
			return true;
		}
		onTick();
		return isWorkDone();
	}

	struct TaskWrapper : public Task
	{
		Task& task;

		TaskWrapper(Task& task)
			: task(task)
		{
		}

		void onTick() SOUP_EXCAL
		{
			if (task.tickUntilDone())
			{
				setWorkDone();
			}
		}

		[[nodiscard]] int getSchedulingDisposition() const noexcept final
		{
			return task.getSchedulingDisposition();
		}
	};

	void Task::run()
	{
		Scheduler sched;
		sched.setDontMakeReusableSockets();
		sched.setAddWorkerCanWaitForeverForAllICare();
		sched.add<TaskWrapper>(*this);
		sched.run();
	}
}
