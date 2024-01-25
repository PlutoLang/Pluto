#include "string.hpp"

#include <fstream>
#include <streambuf>

#include "filesystem.hpp"

namespace soup
{
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
		std::string ret{};
		if (std::filesystem::exists(file))
		{
			std::ifstream t(file, std::ios::binary);

			t.seekg(0, std::ios::end);
			const size_t s = t.tellg();
			t.seekg(0, std::ios::beg);

			ret.reserve(s);
			ret.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
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
