#pragma once

#include "base.hpp"
#if !SOUP_WASM
#include "fwd.hpp"

#include <optional>

#include "Callback.hpp"
#include "MimeMessage.hpp"
#include "HttpResponse.hpp"

namespace soup
{
	class HttpRequest : public MimeMessage
	{
	public:
		bool use_tls = true;
		uint16_t port = 443;
		std::string method{};
		std::string path{};
		bool path_is_encoded = false;

		HttpRequest() = default;
		HttpRequest(std::string method, std::string host, std::string path);
		HttpRequest(std::string host, std::string path);
		HttpRequest(const Uri& uri);

		[[nodiscard]] const std::string& getHost() const;

		void setPath(std::string&& path);
	private:
		void fixPath();
	public:
		void setPayload(std::string payload);

		[[nodiscard]] std::optional<HttpResponse> execute() const; // blocking
		void executeEventStream(void on_event(std::unordered_map<std::string, std::string>&&, const Capture&), Capture&& cap = {}) const; // blocking
		void send(Socket& s) const;
	private:
		static void execute_recvResponse(Socket& s, std::optional<HttpResponse>* resp);
		static void executeEventStream_recv(Socket& s, void* cap);
	public:
		[[nodiscard]] static bool isChallengeResponse(const HttpResponse& res);

		void setClose() noexcept;
		void setKeepAlive() noexcept;

		static void recvResponse(Socket& s, void callback(Socket&, std::optional<HttpResponse>&&, Capture&&), Capture&& cap = {});
		static void recvEventStream(Socket& s, void callback(Socket&, std::unordered_map<std::string, std::string>&&, const Capture&), Capture&& cap = {});
	};
}
#endif
