#pragma once

#include "base.hpp"
#if !SOUP_WASM
#include "fwd.hpp"
#include "type.hpp"

#include "Worker.hpp"

#if SOUP_WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "PrimitiveRaii.hpp"
#include "SocketAddr.hpp"
#include "SocketTlsEncrypter.hpp"
#include "StructMap.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	class Socket : public Worker
	{
	public:
#if SOUP_WINDOWS
		inline static size_t wsa_consumers = 0;
#else
		inline static bool made_linux_not_suck_dick = false;
#endif

#if SOUP_WINDOWS
		using fd_t = ::SOCKET;
#else
		using fd_t = int;
#endif
		PrimitiveRaii<fd_t, (fd_t)-1> fd{};
		StructMap custom_data;
		SocketAddr peer;
		bool remote_closed = false;
		bool dispatched_connection_lost = false;
		bool callback_recv_on_close = false;

		std::string unrecv_buf{};

		SocketTlsEncrypter tls_encrypter_send;
		SocketTlsEncrypter tls_encrypter_recv;

		Socket() noexcept;

		Socket(const Socket&) = delete;

		Socket(Socket&& b) noexcept
			: Worker(WORKER_TYPE_SOCKET)
		{
			onConstruct();
			operator =(std::move(b));
		}

	protected:
		void onConstruct() noexcept;

	public:
		~Socket() noexcept;

		void operator =(const Socket&) = delete;

		Socket& operator =(Socket&& b) noexcept = default;

		[[nodiscard]] constexpr bool hasConnection() const noexcept
		{
			return fd != -1;
		}

		bool init(int af, int type);

		// Resolves the hostname and tries to connect per IPv4, falling back to IPv6 if that fails.
		// 'timeout_ms' defines how long each connection attempt can maximally take and does not include DNS lookup time.
		bool connect(const std::string& host, uint16_t port, unsigned int timeout_ms = 3000) noexcept; // blocking
		bool connect(const dnsResolver& resolver, const std::string& host, uint16_t port, unsigned int timeout_ms = 3000) noexcept; // blocking

		bool connect(const SocketAddr& addr, unsigned int timeout_ms = 3000) noexcept; // blocking
		bool connect(const IpAddr& ip, uint16_t port, unsigned int timeout_ms = 3000) noexcept // blocking
		{
			return connect(SocketAddr(ip, native_u16_t(port)), timeout_ms);
		}

		bool kickOffConnect(const SocketAddr& addr) noexcept;
		bool kickOffConnect(const IpAddr& ip, uint16_t port) noexcept;

		template <typename T = int>
		bool setOpt(int level, int optname, T&& val) noexcept
		{
#if SOUP_WINDOWS
			return setsockopt(fd, level, optname, (const char*)&val, sizeof(T)) != -1;
#else
			return setsockopt(fd, level, optname, &val, sizeof(T)) != -1;
#endif
		}

		[[nodiscard]] static bool isPortLocallyBound(uint16_t port);

		bool bind6(uint16_t port) noexcept;
		bool bind4(uint16_t port) noexcept;
		bool udpBind6(uint16_t port) noexcept;
		bool udpBind4(uint16_t port) noexcept;
		bool udpBind(const IpAddr& addr, uint16_t port) noexcept;
		bool bind6(int type, uint16_t port, const IpAddr& addr = {}) noexcept;
		bool bind4(int type, uint16_t port, const IpAddr& addr = {}) noexcept;

		[[nodiscard]] Socket accept6() noexcept;
		[[nodiscard]] Socket accept4() noexcept;

		bool setBlocking(bool blocking = true) noexcept;
		bool setNonBlocking() noexcept;

		static bool certchain_validator_none(const X509Certchain&, const std::string&, StructMap&) SOUP_EXCAL; // Accepts everything.
		static bool certchain_validator_default(const X509Certchain&, const std::string&, StructMap&) SOUP_EXCAL;

		void enableCryptoClient(std::string server_name, void(*callback)(Socket&, Capture&&, std::string&& alpn_protocol) SOUP_EXCAL, Capture&& cap = {}, std::string&& initial_application_data = {}, certchain_validator_t certchain_validator = &Socket::certchain_validator_default, std::vector<std::string>&& alpn_protocols = {}) SOUP_EXCAL;
	protected:
		void enableCryptoClientRecvServerHelloDone(UniquePtr<SocketTlsHandshaker>&& handshaker) SOUP_EXCAL;
		void enableCryptoClientProcessServerHelloDone(UniquePtr<SocketTlsHandshaker>&& handshaker) SOUP_EXCAL;
		void enableCryptoServerRecvClientKeyExchangeRsa(UniquePtr<SocketTlsHandshaker>&& handshaker) SOUP_EXCAL;
		void enableCryptoServerRecvClientKeyExchangeEcdhe(UniquePtr<SocketTlsHandshaker>&& handshaker) SOUP_EXCAL;

	public:
		void enableCryptoServer(SharedPtr<CertStore> certstore, void(*callback)(Socket&, Capture&&), Capture&& cap = {}, tls_server_on_client_hello_t on_client_hello = nullptr, tls_server_alpn_select_protocol_t alpn_select_protocol = nullptr);

		// Application Layer

		[[nodiscard]] bool isEncrypted() const noexcept;

		bool send(const std::string& data) SOUP_EXCAL { return send(data.data(), data.size()); }
		bool send(const void* data, size_t size) SOUP_EXCAL;

		bool initUdpBroadcast4();

		bool setSourcePort4(uint16_t port);

		bool udpClientSend(const SocketAddr& addr, const std::string& data) noexcept { return udpClientSend(addr, data.data(), data.size()); }
		bool udpClientSend(const SocketAddr& addr, const char* data, size_t size) noexcept;
		bool udpClientSend(const IpAddr& ip, uint16_t port, const std::string& data) noexcept { return udpClientSend(ip, port, data.data(), data.size()); }
		bool udpClientSend(const IpAddr& ip, uint16_t port, const char* data, size_t size) noexcept;

		bool udpServerSend(const SocketAddr& addr, const std::string& data) noexcept { return udpServerSend(addr, data.data(), data.size()); }
		bool udpServerSend(const SocketAddr& addr, const char* data, size_t size) noexcept;
		bool udpServerSend(const IpAddr& ip, uint16_t port, const std::string& data) noexcept { return udpServerSend(ip, port, data.data(), data.size()); }
		bool udpServerSend(const IpAddr& ip, uint16_t port, const char* data, size_t size) noexcept;

		void recv(void(*callback)(Socket&, std::string&&, Capture&&), Capture&& cap = {}); // 'excal' as long as callback is

		void udpRecv(void(*callback)(Socket&, SocketAddr&&, std::string&&, Capture&&), Capture&& cap = {}) noexcept;

		/*[[nodiscard]] std::string recvExact(int bytes) noexcept
		{
			std::string buf(bytes, 0);
			char* dp = buf.data();
			while (bytes != 0)
			{
				int read = bytes;
				if (!transport_recv(dp, read))
				{
					return {};
				}
				bytes -= read;
				dp += read;
			}
			return buf;
		}*/

		void close() SOUP_EXCAL;

		// TLS - Crypto Layer

		bool tls_sendHandshake(const UniquePtr<SocketTlsHandshaker>& handshaker, TlsHandshakeType_t handshake_type, const std::string& content) SOUP_EXCAL;
		bool tls_sendRecord(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL;
		bool tls_sendRecordEncrypted(TlsContentType_t content_type, const std::string& content) SOUP_EXCAL;
		bool tls_sendRecordEncrypted(TlsContentType_t content_type, const void* data, size_t size) SOUP_EXCAL;

		void tls_recvHandshake(UniquePtr<SocketTlsHandshaker>&& handshaker, void(*callback)(Socket&, UniquePtr<SocketTlsHandshaker>&&, TlsHandshakeType_t, std::string&&), std::string&& pre = {});
		void tls_recvRecord(TlsContentType_t expected_content_type, void(*callback)(Socket&, std::string&&, Capture&&), Capture&& cap = {}); // 'excal' as long as callback is
		void tls_recvRecord(void(*callback)(Socket&, TlsContentType_t, std::string&&, Capture&&), Capture&& cap = {}); // 'excal' as long as callback is

		void tls_close(TlsAlertDescription_t desc) SOUP_EXCAL;

		[[nodiscard]] static std::string tls_alertToCloseReason(const std::string& data);

		// Transport Layer

		bool transport_send(const Buffer<>& buf) const noexcept;
		bool transport_send(const std::string& data) const noexcept;
		bool transport_send(const void* data, int size) const noexcept;

		using transport_recv_callback_t = void(*)(Socket&, std::string&&, Capture&&);

		[[nodiscard]] bool transport_hasData() const;

	protected:
		[[nodiscard]] std::string transport_recvCommon(int max_bytes) SOUP_EXCAL;
	public:
		void transport_recv(transport_recv_callback_t callback, Capture&& cap = {}); // 'excal' as long as callback is
		void transport_recv(int max_bytes, transport_recv_callback_t callback, Capture&& cap = {}); // 'excal' as long as callback is
		void transport_recvExact(int bytes, transport_recv_callback_t callback, Capture&& cap = {}, std::string&& pre = {}); // 'excal' as long as callback is

		void transport_unrecv(const std::string& data) SOUP_EXCAL;

		void transport_close() noexcept;

		// Utils

		[[nodiscard]] bool isWorkDoneOrClosed() const noexcept;

		void keepAlive() SOUP_EXCAL;

		[[nodiscard]] std::string toString() const SOUP_EXCAL;
	};

	// MAY be found in Socket::custom_data. See also: Socket::remote_closed.
	using SocketCloseReason = std::string;
}
#endif
