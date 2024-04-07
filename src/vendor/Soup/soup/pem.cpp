#include "pem.hpp"

#include "base64.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	std::string pem::encode(const std::string& label, const std::string& bin)
	{
		std::string res = "-----BEGIN ";
		res.append(label);
		res.append("-----");
		auto b64 = base64::encode(bin);
		while (!b64.empty())
		{
			res.push_back('\n');
			res.append(b64.substr(0, 64));
			b64.erase(0, 64);
		}
		res.append("\n-----END ");
		res.append(label);
		res.append("-----");
		return res;
	}

	std::string pem::decode(std::string in)
	{
		for (size_t i = 0; i = in.find("-----", i), i != std::string::npos; )
		{
			auto j = i;
			i = in.find("-----", j + 5);
			if (i == std::string::npos)
			{
				break;
			}
			i += 5;
			in.erase(j, i - j);
		}
		return decodeUnpacked(std::move(in));
	}

	std::string pem::decodeUnpacked(std::string in)
	{
		string::erase<std::string>(in, "\r");
		string::erase<std::string>(in, "\n");
		string::erase<std::string>(in, "\t");
		string::erase<std::string>(in, " ");
		return base64::decode(in);
	}

	std::vector<std::string> pem::decodeChain(const std::string& str)
	{
		std::vector<std::string> res{};
		std::string tmp{};
		for (const auto& line : string::explode(str, "\n"))
		{
			if (line.empty())
			{
				continue;
			}
			if (line.at(0) == '-')
			{
				if (!tmp.empty())
				{
					res.emplace_back(pem::decodeUnpacked(tmp));
					tmp.clear();
				}
				continue;
			}
			tmp.append(line);
		}
		return res;
	}
}
