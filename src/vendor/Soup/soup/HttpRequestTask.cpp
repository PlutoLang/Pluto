#include "HttpRequestTask.hpp"
#if !SOUP_WASM || SOUP_EMSCRIPTEN

#if !SOUP_WASM
#define LOGGING false

#if LOGGING
#include "format.hpp"
#include "log.hpp"
#endif

#include "netConfig.hpp"
#include "netStatus.hpp"
#include "ObfusString.hpp"
#include "netReuseTag.hpp"
#include "Scheduler.hpp"
#include "time.hpp"
#else
#include <emscripten/fetch.h> // https://github.com/emscripten-core/emscripten/blob/main/system/include/emscripten/fetch.h
#endif

NAMESPACE_SOUP
{
	HttpRequestTask::HttpRequestTask(const Uri& uri)
		: HttpRequestTask(HttpRequest(uri))
	{
	}

	HttpRequestTask::HttpRequestTask(std::string host, std::string path)
		: HttpRequestTask(HttpRequest(std::move(host), std::move(path)))
	{
	}

#if !SOUP_EMSCRIPTEN
	HttpRequestTask::HttpRequestTask(HttpRequest&& hr)
		: HttpRequestTask(std::move(hr), &Socket::certchain_validator_default)
	{
	}

	HttpRequestTask::HttpRequestTask(HttpRequest&& hr, SharedPtr<dnsResolver> resolver)
		: HttpRequestTask(std::move(hr), resolver, &Socket::certchain_validator_default)
	{
	}

	HttpRequestTask::HttpRequestTask(HttpRequest&& hr, certchain_validator_t certchain_validator)
		: HttpRequestTask(std::move(hr), netConfig::get().getDnsResolver(), certchain_validator)
	{
	}

	HttpRequestTask::HttpRequestTask(HttpRequest&& hr, SharedPtr<dnsResolver> resolver, certchain_validator_t certchain_validator)
		: hr(std::move(hr)), resolver(resolver), certchain_validator(certchain_validator)
	{
	}

	void HttpRequestTask::onTick()
	{
		switch (state)
		{
		case START:
			if (!dont_use_reusable_sockets)
			{
				const auto [host, port] = hr.getHostAndPort();
				sock = Scheduler::get()->findReusableSocket(host, port, hr.use_tls ? (require_ecdhe ? SOCKET_TLS_ECDHE : SOCKET_TLS) : SOCKET_INSECURE);
				if (sock)
				{
					if (sock->custom_data.getStructFromMap(netReuseTag).is_busy)
					{
						state = WAIT_TO_REUSE;
					}
					else
					{
						sendRequestOnReusedSocket();
					}
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
				cannotRecycle();
			}
			else if (!sock->custom_data.getStructFromMap(netReuseTag).is_busy)
			{
				sendRequestOnReusedSocket();
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
				connector.reset();

				// Tag socket we just created for reuse, if it's not a one-off.
				if (dont_make_reusable_sockets == false
					&& Scheduler::get()->dont_make_reusable_sockets == false
					)
				{
					const auto [host, port] = hr.getHostAndPort();
					SOUP_IF_LIKELY (!Scheduler::get()->findReusableSocket(host, port, hr.use_tls ? (require_ecdhe ? SOCKET_TLS_ECDHE : SOCKET_TLS) : SOCKET_INSECURE))
					{
						hr.setKeepAlive();
						sock->custom_data.getStructFromMap(netReuseTag).init(host, port, hr.use_tls ? (require_ecdhe ? SOCKET_TLS_ECDHE : SOCKET_TLS) : SOCKET_INSECURE);
#if LOGGING
						logWriteLine(soup::format("Connected to {} - reusable socket", hr.getHost()));
#endif
					}
#if LOGGING
					else
					{
						logWriteLine(soup::format("Connected to {} - socket will be closed after request is done (duplicate)", hr.getHost()));
					}
#endif
				}
#if LOGGING
				else
				{
					logWriteLine(soup::format("Connected to {} - socket will be closed after request is done (policy)", hr.getHost()));
				}
#endif

				if (hr.use_tls)
				{
					std::string initial_application_data;
					if (hr.body.size() <= 0x4000)
					{
						initial_application_data = hr.getDataToSend();
					}
					sock->enableCryptoClient(std::get<0>(hr.getHostAndPort()), [](Socket& s, Capture&& cap, std::string&&) SOUP_EXCAL
					{
						if (cap.get<HttpRequestTask*>()->hr.body.size() > 0x4000)
						{
							cap.get<HttpRequestTask*>()->sendRequest();
						}
						else
						{
							cap.get<HttpRequestTask*>()->recvResponse();
						}
					}, this, std::move(initial_application_data), certchain_validator, {}, require_ecdhe);
					state = TLS_HANDSHAKE;
				}
				else
				{
					sendRequest();
				}
			}
			break;

		case TLS_HANDSHAKE:
			SOUP_IF_UNLIKELY (sock->isWorkDoneOrClosed())
			{
#if LOGGING
				logWriteLine(soup::format("TLS_HANDSHAKE to {} - socket closed prematurely", hr.getHost()));
#endif
				if (sock->custom_data.isStructInMap(SocketCloseReason))
				{
					state_finish_reason = sock->custom_data.getStructFromMapConst(SocketCloseReason);
				}
				else
				{
					state_finish_reason = netStatusToString(NET_FAIL_L7_PREMATURE_END);
				}
				sock->close();
				sock.reset();
				setWorkDone();
			}
			break;

		case SEND_REQUEST:
			SOUP_IF_UNLIKELY (!sock->sendRetry(overflow_buffer))
			{
#if LOGGING
				logWriteLine(soup::format("SEND_REQUEST to {} - socket closed prematurely", hr.getHost()));
#endif
				sock->close();
				sock.reset();
				setWorkDone();
			}
			else if (overflow_buffer.empty())
			{
				overflow_buffer.shrink_to_fit();
#if LOGGING
				logWriteLine(soup::format("SEND_REQUEST to {} - all bytes transmitted", hr.getHost()));
#endif
				state = AWAIT_RESPONSE;
				await_response_timeout = time::unixSeconds() + FIRST_CHUNK_TIMEOUT_SECS;
			}
			break;

		case AWAIT_RESPONSE:
			if (time::unixSeconds() > await_response_timeout)
			{
#if LOGGING
				logWriteLine(soup::format("AWAIT_RESPONSE from {} - timeout", hr.getHost()));
#endif
				state_finish_reason = netStatusToString(NET_FAIL_L7_TIMEOUT);
				sock->close();
				sock.reset();
				setWorkDone();
			}
			break;
		}
	}

	void HttpRequestTask::sendRequestOnReusedSocket()
	{
		retry_on_broken_pipe = true;
		sock->custom_data.getStructFromMapConst(netReuseTag).is_busy = true;
		hr.setKeepAlive();
		sendRequest();
	}

	void HttpRequestTask::cannotRecycle()
	{
		sock.reset();

		state = CONNECTING;

		const auto [host, port] = hr.getHostAndPort();
		connector.emplace(resolver, host, port, prefer_ipv6);
	}

	void HttpRequestTask::sendRequest() SOUP_EXCAL
	{
		sock->send(hr.getDataToSend(), overflow_buffer);
		if (overflow_buffer.empty())
		{
			state = AWAIT_RESPONSE;
			await_response_timeout = time::unixSeconds() + FIRST_CHUNK_TIMEOUT_SECS;
		}
		else
		{
			state = SEND_REQUEST;
#if LOGGING
			logWriteLine(soup::format("SEND_REQUEST to {} - {} bytes not yet transmitted", hr.getHost(), overflow_buffer.size()));
#endif
		}
		recvResponse(); // we have to be ready to receive either way
	}

	void HttpRequestTask::recvResponse() SOUP_EXCAL
	{
		HttpRequest::recvResponse(*sock, [](Socket&, const std::string&, const Capture& cap)
		{
			cap.get<HttpRequestTask*>()->await_response_timeout = time::unixSeconds() + SUBSEQUENT_CHUNK_TIMEOUT_SECS;
			return true;
		}, [](Socket& s, Optional<HttpResponse>&& res, Capture&& cap) SOUP_EXCAL
		{
			if (res.has_value())
			{
				if (!HttpRequest::isChallengeResponse(*res))
				{
					cap.get<HttpRequestTask*>()->state_finish_reason = netStatusToString(NET_OK);
				}
				else
				{
					cap.get<HttpRequestTask*>()->state_finish_reason = soup::ObfusString("Blocked By Security Solution").str();
					res.reset();
				}
			}
			else
			{
				if (cap.get<HttpRequestTask*>()->retry_on_broken_pipe)
				{
					cap.get<HttpRequestTask*>()->retry_on_broken_pipe = false;
#if LOGGING
					logWriteLine(soup::format("AWAIT_RESPONSE from {} - broken pipe, making a new one", cap.get<HttpRequestTask*>()->hr.getHost()));
#endif
					s.close();
					cap.get<HttpRequestTask*>()->cannotRecycle(); // transition to CONNECTING state
					return;
				}
#if LOGGING
				logWriteLine(soup::format("AWAIT_RESPONSE from {} - request failed", cap.get<HttpRequestTask*>()->hr.getHost()));
#endif
				if (s.custom_data.isStructInMap(SocketCloseReason))
				{
					cap.get<HttpRequestTask*>()->state_finish_reason = s.custom_data.getStructFromMapConst(SocketCloseReason);
				}
				else
				{
					cap.get<HttpRequestTask*>()->state_finish_reason = netStatusToString(NET_FAIL_L7_PREMATURE_END);
				}
			}
			cap.get<HttpRequestTask*>()->fulfil(std::move(res));
			if (s.custom_data.isStructInMap(netReuseTag))
			{
				if (Scheduler::get()->dont_make_reusable_sockets == false) // Scheduler policy hasn't changed?
				{
					s.custom_data.getStructFromMap(netReuseTag).is_busy = false;
					s.keepAlive();
					return;
				}
			}
			// Not a reusable socket or scheduler policy has changed.
			s.close();
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

		case TLS_HANDSHAKE: str.append(ObfusString("TLS_HANDSHAKE").str()); break;
		case SEND_REQUEST: str.append(ObfusString("SEND_REQUEST").str()); break;
		case AWAIT_RESPONSE: str.append(ObfusString("AWAIT_RESPONSE").str()); break;
		}
		return str;
	}

	std::string HttpRequestTask::getStatus() const SOUP_EXCAL
	{
		switch (state)
		{
		case START:
		case WAIT_TO_REUSE:
			return netStatusToString(NET_PENDING);

		case CONNECTING:
			return netStatusToString(connector->getStatus());

		case SEND_REQUEST:
			return netStatusToString(isWorkDone() ? NET_FAIL_L7_PREMATURE_END : NET_PENDING);

		case TLS_HANDSHAKE:
		case AWAIT_RESPONSE:
			return isWorkDone() ? state_finish_reason : netStatusToString(NET_PENDING);
		}
		SOUP_UNREACHABLE;
	}
#else
	HttpRequestTask::HttpRequestTask(HttpRequest&& _hr)
		: hr(std::move(_hr))
	{
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		if ((hr.method.size() + 1) < sizeof(attr.requestMethod))
		{
			strcpy(attr.requestMethod, hr.method.c_str());
		}
		else
		{
			strcpy(attr.requestMethod, "GET");
		}
		attr.userData = this;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.onsuccess = [](emscripten_fetch_t* fetch)
		{
			HttpResponse resp;
			resp.body = std::string(fetch->data, fetch->numBytes);
			resp.status_code = fetch->status;
			resp.status_text = fetch->statusText;
			((HttpRequestTask*)fetch->userData)->fulfil(std::move(resp));
			emscripten_fetch_close(fetch);
		};
		attr.onerror = [](emscripten_fetch_t* fetch)
		{
			((HttpRequestTask*)fetch->userData)->setWorkDone();
			emscripten_fetch_close(fetch);
		};
		header_fields = hr.getHeaderFields();
		for (const auto& field : header_fields)
		{
			if (field.first != "Host"
				&& field.first != "User-Agent"
				&& field.first != "Connection"
				&& field.first != "Accept-Encoding"
				)
			{
				headers.emplace_back(field.first.c_str());
				headers.emplace_back(field.second.c_str());
			}
		}
		if (!headers.empty())
		{
			headers.emplace_back(nullptr);
			attr.requestHeaders = &headers[0];
		}
		if (!hr.body.empty())
		{
			attr.requestData = hr.body.data();
			attr.requestDataSize = hr.body.size();
		}
		auto url = hr.getUrl();
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

#endif
