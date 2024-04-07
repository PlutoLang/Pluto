#include "Worker.hpp"

#include "Promise.hpp"
#include "Socket.hpp"
#include "Task.hpp"

NAMESPACE_SOUP
{
	void Worker::awaitPromiseCompletion(PromiseBase* p, void(*f)(Worker&, Capture&&), Capture&& cap)
	{
		if (!p->isPending() && canRecurse())
		{
			f(*this, std::move(cap));
		}
		else
		{
			holdup_type = PROMISE_BASE;
			holdup_callback.set(f, std::move(cap));
			holdup_data = p;
		}
	}

	void Worker::awaitPromiseCompletion(Promise<void>* p, void(*f)(Worker&, Capture&&), Capture&& cap)
	{
		if (!p->isPending() && canRecurse())
		{
			f(*this, std::move(cap));
		}
		else
		{
			holdup_type = PROMISE_VOID;
			holdup_callback.set(f, std::move(cap));
			holdup_data = p;
		}
	}

	std::string Worker::toString() const SOUP_EXCAL
	{
#if !SOUP_WASM
		if (type == WORKER_TYPE_SOCKET)
		{
			return static_cast<const Socket*>(this)->toString();
		}
#endif
		if (type == WORKER_TYPE_TASK)
		{
			auto str = static_cast<const Task*>(this)->toString();
			str.insert(0, 1, '[');
			str.push_back(']');
			return str;
		}
		return "[Worker]";
	}
}
