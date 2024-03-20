#pragma once

#include "Scheduler.hpp"
#if !SOUP_WASM

#include "fwd.hpp"
#include "type.hpp"

namespace soup
{
	class Server : public Scheduler
	{
	public:
		using udp_callback_t = void(*)(Socket&, SocketAddr&&, std::string&&) SOUP_EXCAL;

		bool bind(uint16_t port, ServerService* service) noexcept;
		bool bindCrypto(uint16_t port, ServerService* service, tls_server_cert_selector_t cert_selector, tls_server_on_client_hello_t on_client_hello = nullptr) noexcept;
		bool bindUdp(uint16_t port, udp_callback_t callback) noexcept;
		bool bindUdp(const IpAddr& addr, uint16_t port, udp_callback_t callback) noexcept;
		bool bindUdp(uint16_t port, ServerServiceUdp* service) noexcept;
		bool bindUdp(const IpAddr& addr, uint16_t port, ServerServiceUdp* service) noexcept;
	protected:
		static void setDataAvailableHandler6(Socket& s) noexcept;
		static void setDataAvailableHandlerCrypto6(Socket& s) noexcept;
#if SOUP_WINDOWS
		static void setDataAvailableHandler4(Socket& s) noexcept;
		static void setDataAvailableHandlerCrypto4(Socket& s) noexcept;
#endif
		static void setDataAvailableHandlerUdp(Socket& s, udp_callback_t callback) noexcept;
		static void setDataAvailableHandlerUdp(Socket& s, ServerServiceUdp* service) noexcept;
	};
}
#endif
