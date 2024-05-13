#pragma once

#include "Task.hpp"

#include "base.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Uri.hpp"
#if !SOUP_WASM
#include "DelayedCtor.hpp"
#include "netConnectTask.hpp"
#include "SharedPtr.hpp"
#endif

NAMESPACE_SOUP
{
	class HttpRequestTask : public PromiseTask<Optional<HttpResponse>>
	{
	public:
#if !SOUP_WASM
		enum State : uint8_t
		{
			START = 0,
			WAIT_TO_REUSE,
			CONNECTING,
			AWAIT_RESPONSE,
		};

		State state = START;
		bool prefer_ipv6 = false; // for funny things like https://api.lovense.com/api/lan/getToys
		bool dont_use_reusable_sockets = false;
		bool dont_make_reusable_sockets = false;
		bool retry_on_broken_pipe = false; // internal
#endif
		HttpRequest hr;
#if !SOUP_WASM
		DelayedCtor<netConnectTask> connector;
		SharedPtr<Socket> sock;
		time_t awaiting_response_since;
#else
		std::vector<const char*> headers;
#endif

		HttpRequestTask(HttpRequest&& hr);
		HttpRequestTask(const Uri& uri);
		HttpRequestTask(std::string host, std::string path);

#if !SOUP_WASM
		void onTick() final;

	protected:
		void sendRequestOnReusedSocket();
		void cannotRecycle();

		void recvResponse() SOUP_EXCAL;

	public:
		[[nodiscard]] std::string toString() const SOUP_EXCAL final;
		[[nodiscard]] netStatus getStatus() const noexcept;
#else
		void onTick() noexcept final;

		int getSchedulingDisposition() const noexcept final;
#endif
	};
}
