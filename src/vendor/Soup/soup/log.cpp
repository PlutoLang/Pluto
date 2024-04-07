#include "log.hpp"

#include "logStdSink.hpp"

NAMESPACE_SOUP
{
	UniquePtr<logSink> g_logSink = soup::make_unique<logStdSink>();
}
