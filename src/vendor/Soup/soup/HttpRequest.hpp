#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "type.hpp"

#include "Callback.hpp"
#include "HttpResponse.hpp"
#include "MimeMessage.hpp"
#include "Optional.hpp"

NAMESPACE_SOUP
{
	class HttpRequest : public MimeMessage
	{
	public:
		bool use_tls = true;
		bool path_is_encoded = false;
		std::string method{};
		std::string path{};

		HttpRequest() = default;
		HttpRequest(std::string method, const std::string& host, std::string path);
		HttpRequest(const std::string& host, std::string path);
		HttpRequest(const Uri& uri);

		[[nodiscard]] std::string getHost() const; // host[:port]
		[[nodiscard]] std::tuple<std::string, uint16_t> getHostAndPort() const;
		[[nodiscard]] std::string getUrl() const;

		void setPath(std::string&& path);
	private:
		void fixPath();
	public:
		void setPayload(std::string payload);

#if !SOUP_WASM
		[[nodiscard]] Optional<HttpResponse> execute() const; // blocking
		[[nodiscard]] Optional<HttpResponse> execute(certchain_validator_t certchain_validator) const; // blocking
		[[nodiscard]] Optional<HttpResponse> execute(const dnsResolver& resolver) const; // blocking
		[[nodiscard]] Optional<HttpResponse> execute(const dnsResolver& resolver, certchain_validator_t certchain_validator) const; // blocking
		void executeEventStream(void on_event(std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL, Capture&& cap = {}) const; // blocking
		void executeEventStream(const dnsResolver& resolver, void on_event(std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL, Capture&& cap = {}) const; // blocking
		[[nodiscard]] std::string getDataToSend() const SOUP_EXCAL;
		void send(Socket& s) const SOUP_EXCAL;
	private:
		static void execute_recvResponse(Socket& s, Optional<HttpResponse>* resp) SOUP_EXCAL;
		static void executeEventStream_recv(Socket& s, void* cap) SOUP_EXCAL;
	public:
		[[nodiscard]] static bool isChallengeResponse(const HttpResponse& res) SOUP_EXCAL;

		[[deprecated("HttpRequest is already 'Connection: close' by default.")]] void setClose() noexcept;
		void setKeepAlive() noexcept;

		static void recvResponse(Socket& s, void callback(Socket&, Optional<HttpResponse>&&, Capture&&) SOUP_EXCAL, Capture&& cap = {}) SOUP_EXCAL;
		static void recvEventStream(Socket& s, void callback(Socket&, std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL, Capture&& cap = {}) SOUP_EXCAL;
#endif
	};
}
