#include "log.hpp"

#include "logStdSink.hpp"

namespace soup
{
	UniquePtr<logSink> g_logSink = soup::make_unique<logStdSink>();
}
