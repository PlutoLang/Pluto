#pragma once

#include "base.hpp"
#if !SOUP_WASM
#include "fwd.hpp"
#include "type.hpp"

#include "Worker.hpp"

#if SOUP_WINDOWS
#include <Winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "PrimitiveRaii.hpp"
#include "SocketAddr.hpp"
#include "SocketTlsEncrypter.hpp"
#include "StructMap.hpp"
#include "UniquePtr.hpp"

namespace soup
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
		SocketAddr peer;
		StructMap custom_data;
		bool remote_closed = false;
		bool dispatched_connection_lost = false;
		bool callback_recv_on_close = false;

		std::string tls_record_buf{};

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

		bool connect(const char* host, uint16_t port) noexcept; // blocking
		bool connect(const std::string& host, uint16_t port) noexcept; // blocking
		bool connect(const SocketAddr& addr) noexcept; // blocking
		bool connect(const IpAddr& ip, uint16_t port) noexcept; // blocking
		bool kickOffConnect(const SocketAddr& addr) noexcept;
		bool kickOffConnect(const IpAddr& ip, uint16_t port) noexcept;

		template <typename T = int>
		bool setOpt(int name, T&& val) noexcept
		{
#if SOUP_WINDOWS
			return setsockopt(fd, SOL_SOCKET, name, (const char*)&val, sizeof(T)) != -1;
#else
			return setsockopt(fd, SOL_SOCKET, name, &val, sizeof(T)) != -1;
#endif
		}

		[[nodiscard]] static bool isPortLocallyBound(uint16_t port);

		bool bind6(uint16_t port) noexcept;
		bool bind4(uint16_t port) noexcept;
		bool udpBind6(uint16_t port) noexcept;
		bool udpBind4(uint16_t port) noexcept;
		bool udpBind(const IpAddr& addr, uint16_t port) noexcept;
	protected:
		bool bind6(int type, uint16_t port, const IpAddr& addr = {}) noexcept;
		bool bind4(int type, uint16_t port, const IpAddr& addr = {}) noexcept;

	public:
		[[nodiscard]] Socket accept6() noexcept;
		[[nodiscard]] Socket accept4() noexcept;

		bool setBlocking(bool blocking = true) noexcept;
		bool setNonBlocking() noexcept;

		static bool certchain_validator_none(const X509Certchain&, const std::string&, StructMap&); // Accepts everything.
		static bool certchain_validator_relaxed(const X509Certchain&, const std::string&, StructMap&); // [Default] Verifies what can be verified, accepts the rest.
		static bool certchain_validator_strict(const X509Certchain&, const std::string&, StructMap&); // Same as relaxed, but rejects SHA384 & SHA512 because Soup doesn't implement them yet.

		void enableCryptoClient(std::string server_name, void(*callback)(Socket&, Capture&&), Capture&& cap = {});
	protected:
		void enableCryptoClientRecvServerHelloDone(UniquePtr<SocketTlsHandshaker>&& handshaker);
		void enableCryptoClientProcessServerHelloDone(UniquePtr<SocketTlsHandshaker>&& handshaker);

	public:
		void enableCryptoServer(tls_server_cert_selector_t cert_selector, void(*callback)(Socket&, Capture&&), Capture&& cap = {}, tls_server_on_client_hello_t on_client_hello = nullptr);

		// Application Layer

		[[nodiscard]] bool isEncrypted() const noexcept;

		bool send(const std::string& data);

		bool initUdpBroadcast4();

		bool setSourcePort4(uint16_t port);

		bool udpClientSend(const SocketAddr& addr, const std::string& data) noexcept;
		bool udpClientSend(const IpAddr& ip, uint16_t port, const std::string& data) noexcept;

		bool udpServerSend(const SocketAddr& addr, const std::string& data) noexcept;
		bool udpServerSend(const IpAddr& ip, uint16_t port, const std::string& data) noexcept;

		void recv(void(*callback)(Socket&, std::string&&, Capture&&), Capture&& cap = {});

		void udpRecv(void(*callback)(Socket&, SocketAddr&&, std::string&&, Capture&&), Capture&& cap = {});

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

		void close();

		// TLS - Crypto Layer

		bool tls_sendHandshake(const UniquePtr<SocketTlsHandshaker>& handshaker, TlsHandshakeType_t handshake_type, const std::string& content);
		bool tls_sendRecord(TlsContentType_t content_type, const std::string& content);
		bool tls_sendRecordEncrypted(TlsContentType_t content_type, const std::string& content);

		void tls_recvHandshake(UniquePtr<SocketTlsHandshaker>&& handshaker, void(*callback)(Socket&, UniquePtr<SocketTlsHandshaker>&&, TlsHandshakeType_t, std::string&&), std::string&& pre = {});
		void tls_recvRecord(TlsContentType_t expected_content_type, void(*callback)(Socket&, std::string&&, Capture&&), Capture&& cap = {});
		void tls_recvRecord(void(*callback)(Socket&, TlsContentType_t, std::string&&, Capture&&), Capture&& cap = {});

		void tls_close(TlsAlertDescription_t desc);

		// Transport Layer

		bool transport_send(const Buffer& buf) const noexcept;
		bool transport_send(const std::string& data) const noexcept;
		bool transport_send(const void* data, int size) const noexcept;

		using transport_recv_callback_t = void(*)(Socket&, std::string&&, Capture&&);

		[[nodiscard]] bool transport_hasData() const;

	protected:
		[[nodiscard]] std::string transport_recvCommon(int max_bytes);
	public:
		void transport_recv(int max_bytes, transport_recv_callback_t callback, Capture&& cap = {});
		void transport_recvExact(int bytes, transport_recv_callback_t callback, Capture&& cap = {}, std::string&& pre = {});

		void transport_close() noexcept;

		// Utils

		[[nodiscard]] bool isWorkDoneOrClosed() const noexcept;

		void keepAlive();

		[[nodiscard]] std::string toString() const;
	};
}
#endif
