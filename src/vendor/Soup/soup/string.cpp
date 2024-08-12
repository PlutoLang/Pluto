#include "string.hpp"

#include <fstream>
#include <streambuf>

#include "filesystem.hpp"

NAMESPACE_SOUP
{
	std::string string::hex2bin(const std::string& hex) SOUP_EXCAL
	{
		std::string bin;
		uint8_t val = 0;
		bool first_nibble = true;
		for (const auto& c : hex)
		{
			if (isNumberChar(c))
			{
				val |= (c - '0');
			}
			else if (c >= 'a' && c <= 'f')
			{
				val |= 0xA + (c - 'a');
			}
			else if (c >= 'A' && c <= 'F')
			{
				val |= 0xA + (c - 'A');
			}
			else
			{
				continue;
			}
			if (first_nibble)
			{
				val <<= 4;
				first_nibble = false;
			}
			else
			{
				bin.push_back(val);
				val = 0;
				first_nibble = true;
			}
		}
		return bin;
	}

	std::string string::escape(const std::string& str)
	{
		std::string res;

		res.reserve(str.size() + 2);
		res.insert(0, 1, ' ');
		res.append(str);

		string::replaceAll(res, "\\", "\\\\");
		string::replaceAll(res, "\"", "\\\"");

		res.at(0) = '\"';
		res.push_back('\"');

		return res;
	}

	std::string string::join(const std::vector<std::string>& arr, const char glue)
	{
		std::string res{};
		if (!arr.empty())
		{
			res = arr.at(0);
			for (size_t i = 1; i != arr.size(); ++i)
			{
				res.push_back(glue);
				res.append(arr.at(i));
			}
		}
		return res;
	}
	
	std::string string::join(const std::vector<std::string>& arr, const std::string& glue)
	{
		std::string res{};
		if (!arr.empty())
		{
			res = arr.at(0);
			for (size_t i = 1; i != arr.size(); ++i)
			{
				res.append(glue);
				res.append(arr.at(i));
			}
		}
		return res;
	}

	void string::listAppend(std::string& str, std::string&& add)
	{
		if (str.empty())
		{
			str = std::move(add);
		}
		else
		{
			str.append(", ").append(add);
		}
	}

	std::string string::_xor(const std::string& l, const std::string& r)
	{
		if (l.size() < r.size())
		{
			return _xor(r, l);
		}
		// l.size() >= r.size()
		std::string res(l.size(), '\0');
		for (size_t i = 0; i != l.size(); ++i)
		{
			res.at(i) = (char)((uint8_t)l.at(i) ^ (uint8_t)r.at(i % r.size()));
		}
		return res;
	}

	std::string string::xorSameLength(const std::string& l, const std::string& r)
	{
		std::string res(l.size(), '\0');
		for (size_t i = 0; i != l.size(); ++i)
		{
			res.at(i) = (char)((uint8_t)l.at(i) ^ (uint8_t)r.at(i));
		}
		return res;
	}

	std::string string::fromFile(const char* file)
	{
		return fromFile(soup::filesystem::u8path(file));
	}

	std::string string::fromFile(const std::string& file)
	{
		return fromFile(soup::filesystem::u8path(file));
	}

	std::string string::fromFile(const std::filesystem::path& file)
	{
		std::string ret;
		if (std::filesystem::exists(file))
		{
#if SOUP_WINDOWS // kinda messes with hwHid on Linux, also unsure if memory mapping is faster than direct file access on Linux.
			size_t len;
			if (auto addr = soup::filesystem::createFileMapping(file, len))
			{
				ret = std::string((const char*)addr, len);
				soup::filesystem::destroyFileMapping(addr, len);
			}
			else // File might be open in another process, causing memory mapping to fail.
#endif
			{
				std::ifstream t(file, std::ios::binary);

				t.seekg(0, std::ios::end);
				const auto s = static_cast<size_t>(t.tellg());
				t.seekg(0, std::ios::beg);

				ret.reserve(s);
				ret.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			}
		}
		return ret;
	}

	void string::toFile(const char* file, const std::string& contents)
	{
		return toFile(soup::filesystem::u8path(file), contents);
	}

	void string::toFile(const std::string& file, const std::string& contents)
	{
		return toFile(soup::filesystem::u8path(file), contents);
	}

	void string::toFile(const std::filesystem::path& file, const std::string& contents)
	{
		std::ofstream of(file, std::ios_base::binary);
		of << contents;
	}
}
