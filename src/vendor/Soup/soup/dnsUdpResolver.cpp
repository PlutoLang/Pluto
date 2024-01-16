#include "dnsUdpResolver.hpp"

#if !SOUP_WASM

#include "dnsHeader.hpp"
#include "rand.hpp"
#include "Scheduler.hpp"
#include "Socket.hpp"

namespace soup
{
	struct CaptureUdpLookup
	{
		uint16_t id;
		std::string res;

		void recv(Socket& s) noexcept
		{
			s.udpRecv([](Socket& s, SocketAddr&&, std::string&& data, Capture&& _cap)
			{
				CaptureUdpLookup& cap = *_cap.get<CaptureUdpLookup*>();

				dnsHeader dh;
				dh.fromBinary(data);

				if (cap.id == dh.id)
				{
					cap.res = std::move(data);
				}
				else
				{
					cap.recv(s);
				}
			}, this);
		}
	};

	std::vector<UniquePtr<dnsRecord>> dnsUdpResolver::lookup(dnsType qtype, const std::string& name) const
	{
		{
			std::vector<UniquePtr<dnsRecord>> res;
			if (checkBuiltinResult(res, qtype, name))
			{
				return res;
			}
		}

		CaptureUdpLookup data;
		data.id = soup::rand.t<uint16_t>(0, -1);

		Socket sock;
		if (!sock.udpClientSend(server, getQuery(qtype, name, data.id)))
		{
			return {};
		}
		Scheduler sched;
		data.recv(*sched.addSocket(std::move(sock)));
		sched.runFor(timeout_ms);
		return parseResponse(std::move(data.res));
	}
}

#endif
