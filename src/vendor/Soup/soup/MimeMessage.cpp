#include "MimeMessage.hpp"

#include "deflate.hpp"
#include "joaat.hpp"
#include "MimeType.hpp"
#include "ObfusString.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	MimeMessage::MimeMessage(std::unordered_map<std::string, std::string>&& header_fields, std::string&& body)
		: header_fields(std::move(header_fields)), body(std::move(body))
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
		if (auto enc = header_fields.find(ObfusString("Content-Encoding")); enc != header_fields.end())
		{
			auto enc_joaat = joaat::hash(enc->second);
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
		header_fields.emplace(ObfusString("Content-Length"), std::to_string(body.size()));
	}

	void MimeMessage::setContentType()
	{
		if (body.length() > 4
			&& body.substr(0, 4) == "\x89PNG"
			)
		{
			header_fields.emplace("Content-Type", MimeType::IMAGE_PNG);
			return;
		}

		if (body.find("<body>") != std::string::npos)
		{
			header_fields.emplace("Content-Type", MimeType::TEXT_HTML);
			return;
		}

		header_fields.emplace("Content-Type", MimeType::TEXT_PLAIN);
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

	bool MimeMessage::hasHeader(const std::string& key) const noexcept
	{
#if SOUP_CPP20
		return header_fields.contains(key);
#else
		return header_fields.find(key) != header_fields.end();
#endif
	}

	std::string* MimeMessage::findHeader(std::string key) noexcept
	{
		string::lower(key);
		for (auto& e : header_fields)
		{
			if (string::lower(std::string(e.first)) == key)
			{
				return &e.second;
			}
		}
		return nullptr;
	}

	void MimeMessage::addHeader(const std::string& line) SOUP_EXCAL
	{
		if (auto key_offset = line.find(": "); key_offset != std::string::npos)
		{
			auto value = line.substr(key_offset + 2);
			string::trim(value);
			header_fields.emplace(normaliseHeaderCasing(line.substr(0, key_offset)), std::move(value));
		}
	}

	void MimeMessage::setHeader(const std::string& key, const std::string& value) SOUP_EXCAL
	{
		if (auto e = header_fields.find(key); e != header_fields.end())
		{
			e->second = value;
		}
		else
		{
			header_fields.emplace(key, value);
		}
	}

	std::string MimeMessage::normaliseHeaderCasing(const std::string& key) SOUP_EXCAL
	{
		std::string out;
		auto parts = string::explode(key, '-');
		for (auto i = parts.begin(); i != parts.end(); ++i)
		{
			out.reserve(i->size() + 1);
			out.push_back(string::upper_char(i->at(0)));
			out.append(string::lower(i->substr(1)));
			if ((i + 1) != parts.end())
			{
				out.push_back('-');
			}
		}
		return out;
	}

	void MimeMessage::decode()
	{
		if (auto enc = header_fields.find(ObfusString("Content-Encoding")); enc != header_fields.end())
		{
			auto enc_joaat = joaat::hash(enc->second);
			switch (enc_joaat)
			{
			case joaat::hash("gzip"):
			case joaat::hash("deflate"):
				header_fields.erase(enc);
				body = deflate::decompress(body).decompressed;
				break;
			}
		}
	}

	std::string MimeMessage::toString() const
	{
		std::string res{};
		for (const auto& field : header_fields)
		{
			res.append(field.first).append(": ").append(field.second).append("\r\n");
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
