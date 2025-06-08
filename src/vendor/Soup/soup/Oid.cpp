#include "Oid.hpp"

#include <cstring> // memcmp
#include <istream>

#include "MemoryRefReader.hpp"
#include "StringWriter.hpp"
#include "utility.hpp" // SOUP_MOVE_RETURN

NAMESPACE_SOUP
{
	Oid Oid::fromBinary(const std::string& str)
	{
		MemoryRefReader s{ str };
		return fromBinary(s);
	}

	Oid Oid::fromBinary(Reader& r)
	{
		Oid ret{};
		if (uint8_t first; r.u8(first))
		{
			ret.len = 2;
			ret.first = first / 40;
			ret.second = first % 40;
			while (r.hasMore())
			{
				uint32_t comp;
				r.om<uint32_t>(comp);
				if (ret.len < MAX_SIZE)
				{
					ret.rest[ret.len - 2] = comp;
					ret.len++;
				}
			}
		}
		return ret;
	}

	bool Oid::operator==(const Oid& b) const noexcept
	{
		return len == b.len
			&& first == b.first
			&& second == b.second
			&& memcmp(rest, b.rest, (len - 2) * sizeof(uint32_t)) == 0
			;
	}

	bool Oid::operator!=(const Oid& b) const noexcept
	{
		return !operator==(b);
	}

	std::string Oid::toDer() const
	{
		StringWriter sw;
		sw.data.push_back((char)((first * 40) | second));
		for (uint32_t i = 2; i < len; ++i)
		{
			sw.om<uint32_t>(rest[i - 2]);
		}
		SOUP_MOVE_RETURN(sw.data);
	}

	std::string Oid::toString() const
	{
		std::string str{};
		str.append(std::to_string((uint32_t)first));
		str.push_back('.');
		str.append(std::to_string((uint32_t)second));
		for (uint32_t i = 2; i < len; ++i)
		{
			str.push_back('.');
			str.append(std::to_string(rest[i - 2]));
		}
		return str;
	}

	std::ostream& operator<<(std::ostream& os, const Oid& v)
	{
		return os << v.toString();
	}
}
