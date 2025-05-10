#include "MimeMessage.hpp"

#include "deflate.hpp"
#include "joaat.hpp"
#include "MimeType.hpp"
#include "ObfusString.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	MimeMessage::MimeMessage(std::vector<std::string>&& headers, std::string&& body)
		: headers(std::move(headers)), body(std::move(body))
	{
	}

	MimeMessage::MimeMessage(const std::string& data)
	{
		loadMessage(data);
	}

	void MimeMessage::setBody(std::string body)
	{
		this->body = std::move(body);
		setContentLength();
	}

	std::string MimeMessage::getBody() const
	{
		if (auto enc = findHeader(ObfusString("Content-Encoding")))
		{
			auto enc_joaat = joaat::hash(*enc);
			switch (enc_joaat)
			{
			case joaat::hash("gzip"):
			case joaat::hash("deflate"):
				return deflate::decompress(body).decompressed;
			}
		}
		return body;
	}

	void MimeMessage::setContentLength()
	{
		setHeader(ObfusString("Content-Length"), std::to_string(body.size()));
	}

	void MimeMessage::setContentType()
	{
		if (body.length() > 4
			&& body.substr(0, 4) == "\x89PNG"
			)
		{
			setHeader("Content-Type", MimeType::IMAGE_PNG);
			return;
		}

		if (body.find("<body>") != std::string::npos)
		{
			setHeader("Content-Type", MimeType::TEXT_HTML);
			return;
		}

		setHeader("Content-Type", MimeType::TEXT_PLAIN);
	}

	void MimeMessage::loadMessage(const std::string& data)
	{
		auto headers_end = data.find("\r\n\r\n");
		if (headers_end == std::string::npos)
		{
			headers_end = data.size();
		}

		auto body_begin = headers_end + 4;
		if (body_begin < data.size())
		{
			body = data.substr(body_begin);
		}
		else
		{
			body_begin = data.size();
		}

		auto header = data.substr(0, headers_end);
		body = data.substr(body_begin);

		for (const auto& line : string::explode(header, "\r\n"))
		{
			addHeader(line);
		}
	}

	Optional<std::string> MimeMessage::findHeader(std::string key) const SOUP_EXCAL
	{
		normaliseHeaderCasingInplace(key.data(), key.size());
		for (const auto& header : headers)
		{
			if (header.size() > key.size()
				&& header[key.size()] == ':'
				&& memcmp(header.data(), key.data(), key.size()) == 0
				)
			{
				const auto value_off = key.size() + 2;
				return std::string(header.data() + value_off, header.size() - value_off);
			}
		}
		return Optional<std::string>();
	}

	void MimeMessage::addHeader(std::string line) SOUP_EXCAL
	{
		if (auto key_offset = line.find(": "); key_offset != std::string::npos)
		{
			normaliseHeaderCasingInplace(line.data(), key_offset);
			headers.emplace_back(std::move(line));
		}
	}

	void MimeMessage::addHeader(std::string key, const std::string& value) SOUP_EXCAL
	{
		normaliseHeaderCasingInplace(key.data(), key.size());
		std::string line;
		line.reserve(key.size() + 2 + value.size());
		line.append(key);
		line.append(": ");
		line.append(value);
		headers.emplace_back(std::move(line));
	}

	void MimeMessage::setHeader(std::string key, const std::string& value) SOUP_EXCAL
	{
		normaliseHeaderCasingInplace(key.data(), key.size());
		for (auto& header : headers)
		{
			if (header.size() > key.size()
				&& header[key.size()] == ':'
				&& memcmp(header.data(), key.data(), key.size()) == 0
				)
			{
				const auto value_off = key.size() + 2;
				header = header.substr(0, value_off) + value;
				return;
			}
		}
		std::string line;
		line.reserve(key.size() + 2 + value.size());
		line.append(key);
		line.append(": ");
		line.append(value);
		headers.emplace_back(std::move(line));
	}

	void MimeMessage::removeHeader(std::string key) noexcept
	{
		normaliseHeaderCasingInplace(key.data(), key.size());
		for (auto i = headers.begin(); i != headers.end(); ++i)
		{
			const std::string& header = *i;
			if (header.size() > key.size()
				&& header[key.size()] == ':'
				&& memcmp(header.data(), key.data(), key.size()) == 0
				)
			{
				headers.erase(i);
				break;
			}
		}
	}

	void MimeMessage::normaliseHeaderCasingInplace(char* data, size_t size) noexcept
	{
		bool first = true;
		for (size_t i = 0; i != size; ++i)
		{
			if (first)
			{
				data[i] = string::upper_char(data[i]);
			}
			else
			{
				data[i] = string::lower_char(data[i]);
			}
			first = (data[i] == '-');
		}
	}

	void MimeMessage::decode() SOUP_EXCAL
	{
		ObfusString enc_header_name("Content-Encoding");
		if (auto enc = findHeader(enc_header_name))
		{
			auto enc_joaat = joaat::hash(*enc);
			switch (enc_joaat)
			{
			case joaat::hash("gzip"):
			case joaat::hash("deflate"):
				removeHeader(enc_header_name);
				body = deflate::decompress(body).decompressed;
				break;
			}
		}
	}

	std::unordered_map<std::string, std::string> MimeMessage::getHeaderFields() const SOUP_EXCAL
	{
		std::unordered_map<std::string, std::string> map{};
		for (const auto& header : headers)
		{
			const auto sep = header.find(": ");
			SOUP_DEBUG_ASSERT(sep != std::string::npos);
			auto key = header.substr(0, sep);
			auto value = header.substr(sep + 2);
			if (auto e = map.find(key); e != map.end())
			{
				e->second.append(", ");
				e->second.append(value);
			}
			else
			{
				map.emplace(std::move(key), std::move(value));
			}
		}
		return map;
	}

	std::string MimeMessage::toString() const SOUP_EXCAL
	{
		std::string res{};
		for (const auto& header : headers)
		{
			res.append(header).append("\r\n");
		}
		res.append("\r\n");
		res.append(body);
		return res;
	}

	std::string MimeMessage::getCanonicalisedBody(bool relaxed) const
	{
		std::string cbody = body;

		// Ensure body ends on a single "\r\n"
		if (cbody.length() > 2 && cbody.substr(cbody.length() - 2, 2) == "\r\n")
		{
			while (cbody.length() > 4 && cbody.substr(cbody.length() - 4, 4) == "\r\n\r\n")
			{
				cbody.erase(cbody.length() - 2, 2);
			}
		}
		else
		{
			cbody.append("\r\n");
		}

		if (relaxed)
		{
			// Collapse multiple spaces into a single space
			bool had_space = false;
			for (auto i = cbody.begin(); i != cbody.end();)
			{
				if (*i == ' ')
				{
					if (had_space)
					{
						i = cbody.erase(i);
						continue;
					}
					had_space = true;
				}
				else
				{
					had_space = false;
				}
				++i;
			}
		}

		return cbody;
	}
}
