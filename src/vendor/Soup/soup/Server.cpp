#include "Server.hpp"

#if !SOUP_WASM

#include "CertStore.hpp"
#include "ServerService.hpp"
#include "ServerServiceUdp.hpp"
#include "SharedPtr.hpp"
#include "Socket.hpp"

NAMESPACE_SOUP
{
	struct CaptureServerPort
	{
		Server* server;
		ServerService* service;

		void processAccept(Socket&& sock) const SOUP_EXCAL
		{
			if (sock.hasConnection())
			{
				auto s = server->addSocket(std::move(sock));
				if (service->on_connection_established)
				{
					service->on_connection_established(*s, *service, *server);
				}
				service->on_tunnel_established(*s, *service, *server);
			}
		}
	};

	struct CaptureServerPortCrypto : public CaptureServerPort
	{
		SharedPtr<CertStore> certstore;
		tls_server_on_client_hello_t on_client_hello;

		void processAccept(Socket&& sock) const SOUP_EXCAL
		{
			if (sock.hasConnection())
			{
				auto s = server->addSocket(std::move(sock));
				if (service->on_connection_established)
				{
					service->on_connection_established(*s, *service, *server);
				}
				s->enableCryptoServer(certstore, [](Socket& s, Capture&& _cap) SOUP_EXCAL
				{
					CaptureServerPortCrypto& cap = *_cap.get<CaptureServerPortCrypto*>();
					cap.service->on_tunnel_established(s, *cap.service, *cap.server);
				}, this, on_client_hello);
			}
		}
	};

	bool Server::bind(uint16_t port, ServerService* service) noexcept
	{
		Socket sock6{};
		if (!sock6.bind6(port))
		{
			return false;
		}
		setDataAvailableHandler6(sock6);
		sock6.holdup_callback.cap = CaptureServerPort{ this, service };
		addSocket(std::move(sock6));

#if SOUP_WINDOWS
		Socket sock4{};
		if (!sock4.bind4(port))
		{
			return false;
		}
		setDataAvailableHandler4(sock4);
		sock4.holdup_callback.cap = CaptureServerPort{ this, service };
		addSocket(std::move(sock4));
#endif

		return true;
	}

	bool Server::bindCrypto(uint16_t port, ServerService* service, SharedPtr<CertStore> certstore, tls_server_on_client_hello_t on_client_hello) noexcept
	{
		Socket sock6{};
		if (!sock6.bind6(port))
		{
			return false;
		}
		setDataAvailableHandlerCrypto6(sock6);
		sock6.holdup_callback.cap = CaptureServerPortCrypto{ { this, service }, certstore, on_client_hello };
		addSocket(std::move(sock6));

#if SOUP_WINDOWS
		Socket sock4{};
		if (!sock4.bind4(port))
		{
			return false;
		}
		setDataAvailableHandlerCrypto4(sock4);
		sock4.holdup_callback.cap = CaptureServerPortCrypto{ { this, service }, certstore, on_client_hello };
		addSocket(std::move(sock4));
#endif

		return true;
	}

	bool Server::bindUdp(uint16_t port, udp_callback_t callback) noexcept
	{
		Socket sock6{};
		if (!sock6.udpBind6(port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock6, callback);
		addSocket(std::move(sock6));

#if SOUP_WINDOWS
		Socket sock4{};
		if (!sock4.udpBind4(port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock4, callback);
		addSocket(std::move(sock4));
#endif

		return true;
	}

	bool Server::bindUdp(const IpAddr& addr, uint16_t port, udp_callback_t callback) noexcept
	{
		Socket sock{};
		if (!sock.udpBind(addr, port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock, callback);
		addSocket(std::move(sock));
		return true;
	}

	bool Server::bindUdp(uint16_t port, ServerServiceUdp* service) noexcept
	{
		Socket sock6{};
		if (!sock6.udpBind6(port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock6, service);
		addSocket(std::move(sock6));

#if SOUP_WINDOWS
		Socket sock4{};
		if (!sock4.udpBind4(port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock4, service);
		addSocket(std::move(sock4));
#endif

		return true;
	}

	bool Server::bindUdp(const IpAddr& addr, uint16_t port, ServerServiceUdp* service) noexcept
	{
		Socket sock{};
		if (!sock.udpBind(addr, port))
		{
			return false;
		}
		setDataAvailableHandlerUdp(sock, service);
		addSocket(std::move(sock));
		return true;
	}

	void Server::setDataAvailableHandler6(Socket& s) noexcept
	{
		s.holdup_type = Worker::SOCKET;
		s.holdup_callback.fp = [](Worker& w, Capture&& cap) SOUP_EXCAL
		{
			auto& s = static_cast<Socket&>(w);
			cap.get<CaptureServerPort>().processAccept(s.accept6());
		};
	}

	void Server::setDataAvailableHandlerCrypto6(Socket& s) noexcept
	{
		s.holdup_type = Worker::SOCKET;
		s.holdup_callback.fp = [](Worker& w, Capture&& cap) SOUP_EXCAL
		{
			auto& s = static_cast<Socket&>(w);
			cap.get<CaptureServerPortCrypto>().processAccept(s.accept6());
		};
	}

#if SOUP_WINDOWS
	void Server::setDataAvailableHandler4(Socket& s) noexcept
	{
		s.holdup_type = Worker::SOCKET;
		s.holdup_callback.fp = [](Worker& w, Capture&& cap) SOUP_EXCAL
		{
			auto& s = static_cast<Socket&>(w);
			cap.get<CaptureServerPort>().processAccept(s.accept4());
		};
	}

	void Server::setDataAvailableHandlerCrypto4(Socket& s) noexcept
	{
		s.holdup_type = Worker::SOCKET;
		s.holdup_callback.fp = [](Worker& w, Capture&& cap) SOUP_EXCAL
		{
			auto& s = static_cast<Socket&>(w);
			cap.get<CaptureServerPortCrypto>().processAccept(s.accept4());
		};
	}
#endif

	void Server::setDataAvailableHandlerUdp(Socket& s, udp_callback_t callback) noexcept
	{
		s.udpRecv([](Socket& s, SocketAddr&& sender, std::string&& data, Capture&& cap)
		{
			cap.get<udp_callback_t>()(s, std::move(sender), std::move(data));
			setDataAvailableHandlerUdp(s, cap.get<udp_callback_t>());
		}, callback);
	}

	void Server::setDataAvailableHandlerUdp(Socket& s, ServerServiceUdp* service) noexcept
	{
		s.udpRecv([](Socket& s, SocketAddr&& sender, std::string&& data, Capture&& cap)
		{
			cap.get<ServerServiceUdp*>()->callback(s, std::move(sender), std::move(data), *cap.get<ServerServiceUdp*>());
			setDataAvailableHandlerUdp(s, cap.get<ServerServiceUdp*>());
		}, service);
	}
}

#endif
