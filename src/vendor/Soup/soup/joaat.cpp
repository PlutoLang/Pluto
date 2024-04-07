#include "joaat.hpp"

#include <cstring> // memset

#include "base.hpp"

NAMESPACE_SOUP
{
	uint32_t joaat::hash(const std::string& str, uint32_t initial) noexcept
	{
		uint32_t val = partial(str.data(), str.size(), initial);
		finalise(val);
		return val;
	}

	uint32_t joaat::concat(uint32_t val, const std::string& str) noexcept
	{
		undo_finalise(val);
		return hash(str, val);
	}

	[[nodiscard]] static char joaat_find_last_char(uint32_t val)
	{
		char best_c = 0;
		uint32_t best_c_score = -1;
		for (uint8_t c = 0; c != 0x80; ++c)
		{
			uint32_t score = joaat::undo_partial(val, c);
			if (score < best_c_score)
			{
				best_c = c;
				best_c_score = score;
			}
		}
		return best_c;
	}

	uint32_t joaat::deriveInitial(uint32_t val, const std::string& str)
	{
		val = deriveInitialNoFinalise(val, str);
		finalise(val);
		return val;
	}

	uint32_t joaat::deriveInitialNoFinalise(uint32_t val, const std::string& str)
	{
		undo_finalise(val);
		for (auto i = str.crbegin(); i != str.crend(); ++i)
		{
			undo_partial(val);
			val -= *i;
		}
		return val;
	}

	std::optional<std::string> joaat::reverse_short_key(uint32_t val)
	{
		undo_finalise(val);
		undo_partial(val);

		std::string str{};
		if (val == 0)
		{
			return str;
		}
		for (uint8_t i = 0; i != 3; ++i)
		{
			char c = joaat_find_last_char(val);
			str.insert(0, 1, c);
			val = undo_partial(val, c);
			if (val == 0)
			{
				return str;
			}
		}

		return {};
	}

	static void collide_inc(char* buf, size_t idx, size_t& len)
	{
		if (buf[idx] == '\0')
		{
			buf[idx] = 'a';
			++len;
			return;
		}
		if (buf[idx] == '9')
		{
			collide_inc(buf, idx + 1, len);
			buf[idx] = 'a';
			return;
		}
		if (buf[idx] == 'z')
		{
			buf[idx] = '0';
			return;
		}
		++buf[idx];
	}

#if !SOUP_WINDOWS
#define strncpy_s strncpy
#endif

	std::string joaat::collide(uint32_t val, const char* prefix)
	{
		joaat::undo_finalise(val);

		char buf[100];
		memset(buf, 0, sizeof(buf));

		size_t idx = strlen(prefix);
		size_t len = idx;
		strncpy_s(buf, prefix, len);

		while (joaat::partial(&buf[0], len) != val)
		{
			collide_inc(buf, idx, len);
		}

		return std::string((const char*)&buf[0], len);
	}
}
