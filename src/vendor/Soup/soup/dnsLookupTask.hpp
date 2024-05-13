#pragma once

#include "Task.hpp"

#include <vector>

#include "dns_records.hpp"
#include "Optional.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	using dnsLookupTask = PromiseTask<Optional<std::vector<UniquePtr<dnsRecord>>>>;

	struct dnsCachedResultTask : public dnsLookupTask
	{
		static UniquePtr<dnsCachedResultTask> make(std::vector<UniquePtr<dnsRecord>>&& res) SOUP_EXCAL
		{
			auto task = soup::make_unique<dnsCachedResultTask>();
			task->result = std::move(res);
			task->setWorkDone();
			return task;
		}

		void onTick() final
		{
			SOUP_ASSERT_UNREACHABLE;
		}
	};
}
