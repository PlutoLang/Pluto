#include "dnsUdpResolver.hpp"

#if !SOUP_WASM

#include "dnsHeader.hpp"
#include "rand.hpp"
#include "Scheduler.hpp"
#include "Socket.hpp"

NAMESPACE_SOUP
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

	Optional<std::vector<UniquePtr<dnsRecord>>> dnsUdpResolver::lookup(dnsType qtype, const std::string& name) const
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

		unsigned int remaining_requests = 1 + num_retries;
		do
		{
			Socket sock;
			if (!sock.udpClientSend(server, getQuery(qtype, name, data.id)))
			{
				return {};
			}
			Scheduler sched;
			data.recv(*sched.addSocket(std::move(sock)));
			sched.runFor(timeout_ms);
		} while (data.res.empty() && --remaining_requests);

		if (!data.res.empty())
		{
			return parseResponse(std::move(data.res));
		}
		return std::nullopt;
	}
}

#endif
