#include "HttpRequestTask.hpp"

#if !SOUP_WASM
#include "format.hpp"
#include "log.hpp"
#include "netStatus.hpp"
#include "ObfusString.hpp"
#include "ReuseTag.hpp"
#include "Scheduler.hpp"
#include "time.hpp"
#else
#include <emscripten/fetch.h>
#endif

namespace soup
{
#if !SOUP_WASM
	HttpRequestTask::HttpRequestTask(HttpRequest&& _hr)
		: hr(std::move(_hr))
	{
	}

	HttpRequestTask::HttpRequestTask(const Uri& uri)
		: HttpRequestTask(HttpRequest(uri))
	{
	}

	HttpRequestTask::HttpRequestTask(std::string host, std::string path)
		: HttpRequestTask(HttpRequest(std::move(host), std::move(path)))
	{
	}

	void HttpRequestTask::onTick()
	{
		switch (state)
		{
		case START:
			if (!dont_keep_alive
				&& !Scheduler::get()->dont_make_reusable_sockets
				)
			{
				hr.setKeepAlive();
				sock = Scheduler::get()->findReusableSocket(hr.getHost(), hr.port, hr.use_tls);
				if (sock)
				{
					doRecycle();
					break;
				}
				// Another task to the same remote may be in the CONNECTING state at this point,
				// but it will be faster (or at least the same speed) to just make a one-off socket instead of waiting to resue.
			}
			cannotRecycle(); // transition to CONNECTING state
			break;

		case WAIT_TO_REUSE:
			if (sock->isWorkDoneOrClosed())
			{
				sock.reset();
				cannotRecycle();
			}
			else if (!sock->custom_data.getStructFromMap(ReuseTag).is_busy)
			{
				sock->custom_data.getStructFromMap(ReuseTag).is_busy = true;
				state = AWAIT_RESPONSE;
				awaiting_response_since = time::unixSeconds();
				hr.send(*sock);
				recvResponse();
			}
			break;

		case CONNECTING:
			if (connector->tickUntilDone())
			{
				if (!connector->wasSuccessful())
				{
					setWorkDone();
					return;
				}
				sock = connector->getSocket();
				connector.destroy();
				if (!Scheduler::get()->dont_make_reusable_sockets)
				{
					// Tag socket we just created for reuse, if it's not a one-off.
					SOUP_IF_UNLIKELY (Scheduler::get()->findReusableSocket(hr.getHost(), hr.port, hr.use_tls))
					{
						hr.setClose();
					}
					else
					{
						sock->custom_data.getStructFromMap(ReuseTag).init(hr.getHost(), hr.port, hr.use_tls);
					}
				}
				state = AWAIT_RESPONSE;
				awaiting_response_since = time::unixSeconds();
				if (hr.use_tls)
				{
					sock->enableCryptoClient(hr.getHost(), [](Socket&, Capture&& cap) SOUP_EXCAL
					{
						cap.get<HttpRequestTask*>()->recvResponse();
					}, this, hr.getDataToSend());
				}
				else
				{
					hr.send(*sock);
					recvResponse();
				}
			}
			break;

		case AWAIT_RESPONSE:
			if (sock->isWorkDoneOrClosed())
			{
				sock->close();
				sock.reset();
				if (attempted_reuse)
				{
					//logWriteLine(soup::format("AWAIT_RESPONSE from {} - broken pipe, making a new one", hr.getHost()));
					cannotRecycle(); // transition to CONNECTING state
				}
				else
				{
					//logWriteLine(soup::format("AWAIT_RESPONSE from {} - request failed", hr.getHost()));
					setWorkDone();
				}
			}
			else if (time::unixSecondsSince(awaiting_response_since) > 30)
			{
				//logWriteLine(soup::format("AWAIT_RESPONSE from {} - timeout", hr.getHost()));
				sock->close();
				sock.reset();
				setWorkDone();
			}
			break;
		}
	}

	void HttpRequestTask::doRecycle()
	{
		attempted_reuse = true;
		if (sock->custom_data.getStructFromMap(ReuseTag).is_busy)
		{
			state = WAIT_TO_REUSE;
		}
		else
		{
			sock->custom_data.getStructFromMap(ReuseTag).is_busy = true;
			state = AWAIT_RESPONSE;
			awaiting_response_since = time::unixSeconds();
			hr.send(*sock);
			recvResponse();
		}
	}

	void HttpRequestTask::cannotRecycle()
	{
		state = CONNECTING;
		connector.construct(hr.getHost(), hr.port, prefer_ipv6);
	}

	void HttpRequestTask::recvResponse() SOUP_EXCAL
	{
		HttpRequest::recvResponse(*sock, [](Socket& s, std::optional<HttpResponse>&& res, Capture&& cap) SOUP_EXCAL
		{
			cap.get<HttpRequestTask*>()->fulfil(std::move(res));
			if (s.custom_data.isStructInMap(ReuseTag))
			{
				s.custom_data.getStructFromMap(ReuseTag).is_busy = false;
				s.keepAlive();
			}
		}, this);
	}

	std::string HttpRequestTask::toString() const SOUP_EXCAL
	{
		std::string str = ObfusString("HttpRequestTask");
		str.push_back('(');
		str.append(hr.getHost());
		str.append(hr.path);
		str.push_back(')');
		str.append(": ");
		switch (state)
		{
		case START: str.append(ObfusString("START").str()); break;
		case WAIT_TO_REUSE: str.append(ObfusString("WAIT_TO_REUSE").str()); break;

		case CONNECTING:
			str.append(ObfusString("CONNECTING").str());
			str.append(": ");
			str.push_back('[');
			str.append(connector->toString());
			str.push_back(']');
			break;

		case AWAIT_RESPONSE: str.append(ObfusString("AWAIT_RESPONSE").str()); break;
		}
		return str;
	}

	netStatus HttpRequestTask::getStatus() const noexcept
	{
		switch (state)
		{
		case CONNECTING: return connector->getStatus();
		case AWAIT_RESPONSE: return isWorkDone() ? (result.has_value() ? NET_OK : NET_FAIL_L7_TIMEOUT) : NET_PENDING;
		default: break; // keep the compiler happy
		}
		// Assuming `!isWorkDone()` because the task can only finish during CONNECTING and AWAIT_RESPONSE.
		return NET_PENDING;
	}
#else
	HttpRequestTask::HttpRequestTask(const Uri& uri)
	{
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.userData = this;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.onsuccess = [](emscripten_fetch_t* fetch)
		{
			HttpResponse resp;
			resp.body = std::string(fetch->data, fetch->numBytes);
			resp.status_code = fetch->status;
			((HttpRequestTask*)fetch->userData)->fulfil(std::move(resp));
			emscripten_fetch_close(fetch);
		};
		attr.onerror = [](emscripten_fetch_t* fetch)
		{
			((HttpRequestTask*)fetch->userData)->setWorkDone();
			emscripten_fetch_close(fetch);
		};
		auto url = uri.toString();
		emscripten_fetch(&attr, url.c_str());
	}

	void HttpRequestTask::onTick() noexcept
	{
	}

	int HttpRequestTask::getSchedulingDisposition() const noexcept
	{
		return LOW_FREQUENCY;
	}
#endif
}
