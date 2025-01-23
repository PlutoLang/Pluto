#include "HttpRequest.hpp"

#include "HttpRequestTask.hpp"
#include "joaat.hpp"
#include "ObfusString.hpp"
#include "os.hpp"
#include "Scheduler.hpp"
#include "Socket.hpp"
#include "UniquePtr.hpp"
#include "Uri.hpp"
#include "urlenc.hpp"
#include "utility.hpp" // SOUP_MOVE_RETURN

#define LOGGING false

#if LOGGING
#include "log.hpp"
#endif

NAMESPACE_SOUP
{
	HttpRequest::HttpRequest(std::string method, std::string host, std::string path)
		: MimeMessage({
			{ObfusString("Host"), std::move(host)},
			{ObfusString("User-Agent"), ObfusString("Mozilla/5.0 (compatible; calamity-inc/Soup)")},
			{ObfusString("Connection"), ObfusString("close")},
			{ObfusString("Accept-Encoding"), ObfusString("deflate, gzip")},
		}), method(std::move(method)), path(std::move(path))
	{
		fixPath();
	}

	HttpRequest::HttpRequest(std::string host, std::string path)
		: HttpRequest(ObfusString("GET"), std::move(host), std::move(path))
	{
	}

	HttpRequest::HttpRequest(const Uri& uri)
		: HttpRequest(uri.host, uri.getRequestPath())
	{
		path_is_encoded = true;

		if (joaat::hash(uri.scheme) == joaat::compileTimeHash("http"))
		{
			use_tls = false;
			port = 80;
		}

		if (uri.port != 0)
		{
			port = uri.port;
		}
	}

	const std::string& HttpRequest::getHost() const
	{
		return header_fields.at(ObfusString("Host"));
	}

	std::string HttpRequest::getUrl() const
	{
		std::string str;
		if (use_tls)
		{
			str = "https://";
		}
		else
		{
			str = "http://";
		}
		str.append(getHost());
		if (port != (use_tls ? 443 : 80))
		{
			str.push_back(':');
			str.append(std::to_string(port));
		}
		if (path_is_encoded)
		{
			str.append(path);
		}
		else
		{
			str.append(urlenc::encode(path));
		}
		return str;
	}

	void HttpRequest::setPath(std::string&& path)
	{
		this->path = std::move(path);
		fixPath();
	}

	void HttpRequest::fixPath()
	{
		if (path.c_str()[0] != '/')
		{
			path.insert(0, 1, '/');
		}
	}

	void HttpRequest::setPayload(std::string payload)
	{
		if (joaat::hash(method) == joaat::hash("GET"))
		{
			method = ObfusString("POST").str();
		}

		setBody(std::move(payload));
	}

#if !SOUP_WASM
	struct HttpRequestExecuteData
	{
		const HttpRequest* req;
		Optional<HttpResponse> resp;
	};

	Optional<HttpResponse> HttpRequest::execute(Scheduler* keep_alive_sched) const
	{
		if (keep_alive_sched)
		{
			auto task = keep_alive_sched->add<HttpRequestTask>(HttpRequest(*this));
			do
			{
				os::sleep(1);
			} while (!task->isWorkDone());
			SOUP_MOVE_RETURN(task->result);
		}

		HttpRequestExecuteData data{ this };
		auto sock = make_shared<Socket>();
		const auto& host = getHost();
		if (sock->connect(host, port))
		{
			Scheduler sched{};
			sched.addSocket(sock);
			if (use_tls)
			{
				sock->enableCryptoClient(host, [](Socket& s, Capture&& cap) SOUP_EXCAL
				{
					auto& data = *cap.get<HttpRequestExecuteData*>();
					execute_recvResponse(s, &data.resp);
				}, &data, getDataToSend());
			}
			else
			{
				send(*sock);
				execute_recvResponse(*sock, &data.resp);
			}
			sched.setAddWorkerCanWaitForeverForAllICare();
			sched.run();
		}
		SOUP_MOVE_RETURN(data.resp);
	}

	struct HttpRequestExecuteEventStreamData
	{
		const HttpRequest* req;
		void(*on_event)(std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL;
		Capture cap;
	};

	void HttpRequest::executeEventStream(void on_event(std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL, Capture&& cap) const
	{
		HttpRequestExecuteEventStreamData data{ this, on_event, std::move(cap) };
		auto sock = make_shared<Socket>();
		const auto& host = getHost();
		if (sock->connect(host, port))
		{
			Scheduler sched{};
			sched.addSocket(sock);
			if (use_tls)
			{
				sock->enableCryptoClient(host, [](Socket& s, Capture&& cap) SOUP_EXCAL
				{
					auto* data = cap.get<HttpRequestExecuteEventStreamData*>();
					executeEventStream_recv(s, data);
				}, &data, getDataToSend());
			}
			else
			{
				send(*sock);
				executeEventStream_recv(*sock, &data);
			}
			sched.run();
		}
	}

	std::string HttpRequest::getDataToSend() const SOUP_EXCAL
	{
		std::string data{};
		data.append(method);
		data.push_back(' ');
		data.append(path_is_encoded ? path : urlenc::encodePathWithQuery(path));
		data.append(ObfusString(" HTTP/1.1").str());
		data.append("\r\n");
		data.append(toString());
		return data;
	}

	void HttpRequest::send(Socket& s) const SOUP_EXCAL
	{
		s.send(getDataToSend());
	}

	void HttpRequest::execute_recvResponse(Socket& s, Optional<HttpResponse>* resp) SOUP_EXCAL
	{
		recvResponse(s, [](Socket& s, Optional<HttpResponse>&& resp, Capture&& cap) noexcept
		{
			*cap.get<Optional<HttpResponse>*>() = std::move(resp);
		}, resp);
	}

	void HttpRequest::executeEventStream_recv(Socket& s, void* cap) SOUP_EXCAL
	{
		recvEventStream(s, [](Socket& s, std::unordered_map<std::string, std::string>&& event, const Capture& cap) SOUP_EXCAL
		{
			auto* data = cap.get<HttpRequestExecuteEventStreamData*>();
			data->on_event(std::move(event), data->cap);
		}, cap);
	}

	bool HttpRequest::isChallengeResponse(const HttpResponse& res) SOUP_EXCAL
	{
		return res.body.find(ObfusString(R"(href="https://www.cloudflare.com?utm_source=challenge)").str()) != std::string::npos
			|| res.body.find(ObfusString(R"(<span id="challenge-error-text">Enable JavaScript and cookies to continue</)").str()) != std::string::npos // "Invisible" challenge
			;
	}

	void HttpRequest::setClose() noexcept
	{
		header_fields.at(ObfusString("Connection")) = ObfusString("close").str();
	}

	void HttpRequest::setKeepAlive() noexcept
	{
		header_fields.at(ObfusString("Connection")) = ObfusString("keep-alive").str();
	}

	struct HttpResponseReceiver
	{
		enum Status : uint8_t
		{
			CODE,
			HEADER,
			BODY_CHUNKED,
			BODY_LEN,
			BODY_CLOSE,
		};

		std::string buf{};
		HttpResponse resp{};
		Status status = CODE;
		uint64_t bytes_remain = 0;

		bool(*on_body_part)(Socket&, const std::string&, const Capture&) SOUP_EXCAL = nullptr;
		void(*callback)(Socket&, Optional<HttpResponse>&&, Capture&&) SOUP_EXCAL = nullptr;
		Capture cap;

		HttpResponseReceiver(bool on_body_part(Socket&, const std::string&, const Capture&) SOUP_EXCAL, Capture&& cap) noexcept
			: on_body_part(on_body_part), cap(std::move(cap))
		{
		}

		HttpResponseReceiver(void callback(Socket&, Optional<HttpResponse>&&, Capture&&) SOUP_EXCAL, Capture&& cap) noexcept
			: callback(callback), cap(std::move(cap))
		{
		}

		void tick(Socket& s, Capture&& cap) SOUP_EXCAL
		{
			s.recv([](Socket& s, std::string&& app, Capture&& cap) SOUP_EXCAL
			{
				auto& self = cap.get<HttpResponseReceiver>();
				if (app.empty())
				{
					// Connection was closed and no more data
					if (self.status == BODY_CLOSE)
					{
						self.resp.body = std::move(self.buf);
						self.callbackSuccess(s);
					}
					else
					{
#if LOGGING
						logWriteLine("Connection closed unexpectedly");
#endif
						if (self.callback)
						{
							self.callback(s, std::nullopt, std::move(self.cap));
						}
					}
					return;
				}
				self.buf.append(app);
				while (true)
				{
					if (self.status == CODE)
					{
						auto i = self.buf.find("\r\n");
						if (i == std::string::npos)
						{
							break;
						}
						auto arr = string::explode(self.buf.substr(0, i), ' ');
						if (arr.size() < 2)
						{
							// Invalid data
#if LOGGING
							logWriteLine("Invalid data");
#endif
							if (self.callback)
							{
								self.callback(s, std::nullopt, std::move(self.cap));
							}
							return;
						}
						self.buf.erase(0, i + 2);
						self.resp.status_code = string::toInt<uint16_t>(arr.at(1), 0);
						arr.erase(arr.begin(), arr.begin() + 2);
						self.resp.status_text = string::join(arr, ' ');
						self.status = HEADER;
					}
					else if (self.status == HEADER)
					{
						auto i = self.buf.find("\r\n");
						if (i == std::string::npos)
						{
							break;
						}
						auto line = self.buf.substr(0, i);
						self.buf.erase(0, i + 2);
						SOUP_IF_LIKELY (!line.empty())
						{
							self.resp.addHeader(line);
						}
						else
						{
							if (auto enc = self.resp.header_fields.find(ObfusString("Transfer-Encoding")); enc != self.resp.header_fields.end())
							{
								if (joaat::hash(enc->second) == joaat::hash("chunked"))
								{
									self.status = BODY_CHUNKED;
								}
							}
							if (self.status == HEADER)
							{
								if (auto len = self.resp.header_fields.find(ObfusString("Content-Length")); len != self.resp.header_fields.end())
								{
									self.status = BODY_LEN;
									if (auto opt = string::toIntOpt<uint64_t>(len->second, string::TI_FULL); opt.has_value())
									{
										self.bytes_remain = opt.value();
									}
									else
									{
#if LOGGING
										logWriteLine("Error parsing content length");
#endif
										if (self.callback)
										{
											self.callback(s, std::nullopt, std::move(self.cap));
										}
										return;
									}
								}
								else
								{
									if (auto con = self.resp.header_fields.find(ObfusString("Connection")); con != self.resp.header_fields.end())
									{
										if (joaat::hash(con->second) == joaat::hash("close"))
										{
											self.status = BODY_CLOSE;
											s.callback_recv_on_close = true;
										}
									}
									if (self.status == HEADER)
									{
										// We still have no idea how to read the body. Assuming response is header-only.
										self.callbackSuccess(s);
										return;
									}
								}
							}
						}
					}
					else if (self.status == BODY_CHUNKED)
					{
						if (self.bytes_remain == 0)
						{
							auto i = self.buf.find("\r\n");
							if (i == std::string::npos)
							{
								break;
							}
							if (auto opt = string::hexToIntOpt<uint64_t>(self.buf.substr(0, i)); opt.has_value())
							{
								self.bytes_remain = opt.value();
							}
							else
							{
#if LOGGING
								logWriteLine("Error parsing chunk length");
#endif
								if (self.callback)
								{
									self.callback(s, std::nullopt, std::move(self.cap));
								}
								return;
							}
							self.buf.erase(0, i + 2);
							if (self.bytes_remain == 0)
							{
								self.callbackSuccess(s);
								return;
							}
						}
						else
						{
							if (self.buf.size() < self.bytes_remain + 2)
							{
								break;
							}
							auto chunk = self.buf.substr(0, static_cast<size_t>(self.bytes_remain));
							if (self.on_body_part)
							{
								SOUP_IF_UNLIKELY (!self.on_body_part(s, chunk, self.cap))
								{
									return;
								}
							}
							self.resp.body.append(chunk);
							self.buf.erase(0, static_cast<size_t>(self.bytes_remain + 2));
							self.bytes_remain = 0;
						}
					}
					else if (self.status == BODY_LEN)
					{
						if (self.buf.size() < self.bytes_remain)
						{
							break;
						}
						self.resp.body = self.buf.substr(0, static_cast<size_t>(self.bytes_remain));
						//self.buf.erase(0, self.bytes_remain);
						self.callbackSuccess(s);
						return;
					}
					else if (self.status == BODY_CLOSE)
					{
						if (self.on_body_part)
						{
							SOUP_IF_UNLIKELY (!self.on_body_part(s, app, self.cap))
							{
								return;
							}
						}
						break;
					}
				}
				self.tick(s, std::move(cap));
			}, std::move(cap));
		}

		void callbackSuccess(Socket& s)
		{
			if (!callback)
			{
				return;
			}
			resp.decode();
			SOUP_IF_LIKELY (!HttpRequest::isChallengeResponse(resp))
			{
				callback(s, std::move(resp), std::move(cap));
			}
			else
			{
#if LOGGING
				logWriteLine("Challenge response");
#endif
				callback(s, std::nullopt, std::move(cap));
			}
		}
	};

	void HttpRequest::recvResponse(Socket& s, void callback(Socket&, Optional<HttpResponse>&&, Capture&&) SOUP_EXCAL, Capture&& _cap) SOUP_EXCAL
	{
		Capture cap = HttpResponseReceiver(callback, std::move(_cap));
		cap.get<HttpResponseReceiver>().tick(s, std::move(cap));
	}

	struct CaptureRecvEventStream
	{
		void(*callback)(Socket&, std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL;
		Capture cap;
	};

	void HttpRequest::recvEventStream(Socket& s, void callback(Socket&, std::unordered_map<std::string, std::string>&&, const Capture&) SOUP_EXCAL, Capture&& cap) SOUP_EXCAL
	{
		Capture receiver_cap = HttpResponseReceiver([](Socket& s, const std::string& part, const Capture& _cap) SOUP_EXCAL
		{
			std::unordered_map<std::string, std::string> data{};
			auto lines = string::explode(part, '\n');
			for (auto& line : lines)
			{
				if (line.back() == '\r')
				{
					line.pop_back();
				}
				if (line.empty())
				{
					if (!data.empty())
					{
						auto& cap = _cap.get<CaptureRecvEventStream>();
						cap.callback(s, std::move(data), cap.cap);
						data.clear();
					}
				}
				else
				{
					auto sep = line.find(": ");
					SOUP_IF_UNLIKELY (sep == std::string::npos)
					{
						return false;
					}
					data.emplace(line.substr(0, sep), line.substr(sep + 2));
				}
			}
			return true;
		}, CaptureRecvEventStream{ callback, std::move(cap) });
		receiver_cap.get<HttpResponseReceiver>().tick(s, std::move(receiver_cap));
	}
#endif
}
