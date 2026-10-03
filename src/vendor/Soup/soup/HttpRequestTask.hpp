#pragma once

#include "base.hpp"
#if !SOUP_WASM || SOUP_EMSCRIPTEN
#include "Task.hpp"

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Uri.hpp"
#if !SOUP_WASM
#include "netConnectTask.hpp"
#include "Optional.hpp"
#include "SharedPtr.hpp"
#endif

NAMESPACE_SOUP
{
	class HttpRequestTask : public PromiseTask<Optional<HttpResponse>>
	{
	public:
#if !SOUP_EMSCRIPTEN
		enum State : uint8_t
		{
			START = 0,
			WAIT_TO_REUSE,
			CONNECTING,
			TLS_HANDSHAKE,
			SEND_REQUEST,
			AWAIT_RESPONSE,
		};

		State state = START;
		bool prefer_ipv6 = false; // for funny things like https://api.lovense.com/api/lan/getToys
		bool dont_use_reusable_sockets = false;
		bool dont_make_reusable_sockets = false;
		bool require_ecdhe = false;
		bool retry_on_broken_pipe = false; // internal
		// <2 free bytes due to alignment>
		std::string state_finish_reason; // internal
#endif
		HttpRequest hr;
#if !SOUP_EMSCRIPTEN
		SharedPtr<dnsResolver> resolver;
		certchain_validator_t certchain_validator;
		Optional<netConnectTask> connector;
		SharedPtr<Socket> sock;
		std::string overflow_buffer;
		time_t await_response_timeout;
		static constexpr time_t FIRST_CHUNK_TIMEOUT_SECS = 30;
		static constexpr time_t SUBSEQUENT_CHUNK_TIMEOUT_SECS = 10;
#else
		std::unordered_map<std::string, std::string> header_fields;
		std::vector<const char*> headers;
#endif

		HttpRequestTask(const Uri& uri);
		HttpRequestTask(std::string host, std::string path);
		HttpRequestTask(HttpRequest&& hr);
#if !SOUP_EMSCRIPTEN
		HttpRequestTask(HttpRequest&& hr, SharedPtr<dnsResolver> resolver);
		HttpRequestTask(HttpRequest&& hr, certchain_validator_t certchain_validator);
		HttpRequestTask(HttpRequest&& hr, SharedPtr<dnsResolver> resolver, certchain_validator_t certchain_validator);
#endif

#if !SOUP_EMSCRIPTEN
		void onTick() final;

	protected:
		void sendRequestOnReusedSocket();
		void cannotRecycle();

		void sendRequest() SOUP_EXCAL;
		void recvResponse() SOUP_EXCAL;

	public:
		[[nodiscard]] std::string toString() const SOUP_EXCAL final;
		[[nodiscard]] std::string getStatus() const SOUP_EXCAL;
#else
		void onTick() noexcept final;

		int getSchedulingDisposition() const noexcept final;
#endif
	};
}

#endif
