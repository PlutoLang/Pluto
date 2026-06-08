#pragma once

#include "Scheduler.hpp"
#if !SOUP_WASM

#include "fwd.hpp"
#include "type.hpp"

NAMESPACE_SOUP
{
	class Server : public Scheduler
	{
	public:
		using udp_callback_t = void(*)(Socket&, SocketAddr&&, std::string&&) SOUP_EXCAL;

		uint16_t bind(uint16_t port, ServerService* service) SOUP_EXCAL;
		uint16_t bind(const IpAddr& ip, uint16_t port, ServerService* service) SOUP_EXCAL;
		uint16_t bindCrypto(uint16_t port, ServerService* service, SharedPtr<CertStore> certstore, tls_server_select_ciphersuite_t select_ciphersuite = nullptr, tls_server_alpn_select_protocol_t alpn_select_protocol = nullptr) SOUP_EXCAL;
		uint16_t bindOptCrypto(uint16_t port, ServerService* service, SharedPtr<CertStore> certstore, tls_server_select_ciphersuite_t select_ciphersuite = nullptr, tls_server_alpn_select_protocol_t alpn_select_protocol = nullptr) SOUP_EXCAL;
		uint16_t bindUdp(uint16_t port, udp_callback_t callback) SOUP_EXCAL;
		uint16_t bindUdp(const IpAddr& addr, uint16_t port, udp_callback_t callback) SOUP_EXCAL;
		uint16_t bindUdp(uint16_t port, ServerServiceUdp* service) SOUP_EXCAL;
		uint16_t bindUdp(const IpAddr& addr, uint16_t port, ServerServiceUdp* service) SOUP_EXCAL;
	protected:
		static void setDataAvailableHandler6(Socket& s) noexcept;
		static void setDataAvailableHandlerCrypto6(Socket& s) noexcept;
		static void setDataAvailableHandlerOptCrypto6(Socket& s) noexcept;
#if SOUP_WINDOWS
		static void setDataAvailableHandler4(Socket& s) noexcept;
		static void setDataAvailableHandlerCrypto4(Socket& s) noexcept;
		static void setDataAvailableHandlerOptCrypto4(Socket& s) noexcept;
#endif
		static void setDataAvailableHandlerUdp(Socket& s, udp_callback_t callback) noexcept;
		static void setDataAvailableHandlerUdp(Socket& s, ServerServiceUdp* service) noexcept;
	};
}
#endif
