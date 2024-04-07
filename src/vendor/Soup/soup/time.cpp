#include "time.hpp"

#include "base.hpp"
#include "string.hpp"

#if SOUP_WINDOWS
#define timegm _mkgmtime
#endif

NAMESPACE_SOUP
{
	static const char wdays[(7 * 3) + 1] = "SunMonTueWedThuFriSat";
	static const char months[(12 * 3) + 1] = "JanFebMarAprMayJunJulAugSepOctNovDec";

	Datetime Datetime::fromTm(const struct tm& t) noexcept
	{
		Datetime dt;

		dt.year = t.tm_year + 1900;
		dt.month = t.tm_mon + 1;
		dt.day = t.tm_mday;
		dt.hour = t.tm_hour;
		dt.minute = t.tm_min;
		dt.second = t.tm_sec;

		dt.wday = t.tm_wday;

		return dt;
	}

	std::optional<Datetime> Datetime::fromIso8601(const char* str) noexcept
	{
		Datetime dt{};
		for (; string::isNumberChar(*str); ++str)
		{
			dt.year *= 10;
			dt.year += ((*str) - '0');
		}
		if (*str != '-')
		{
			return std::nullopt;
		}
		++str;
		for (; string::isNumberChar(*str); ++str)
		{
			dt.month *= 10;
			dt.month += ((*str) - '0');
		}
		if (*str != '-')
		{
			return std::nullopt;
		}
		++str;
		for (; string::isNumberChar(*str); ++str)
		{
			dt.day *= 10;
			dt.day += ((*str) - '0');
		}
		if (*str == 'T')
		{
			++str;
			for (; string::isNumberChar(*str); ++str)
			{
				dt.hour *= 10;
				dt.hour += ((*str) - '0');
			}
			if (*str != ':')
			{
				return std::nullopt;
			}
			++str;
			for (; string::isNumberChar(*str); ++str)
			{
				dt.minute *= 10;
				dt.minute += ((*str) - '0');
			}
			if (*str == ':')
			{
				++str;
				for (; string::isNumberChar(*str); ++str)
				{
					dt.second *= 10;
					dt.second += ((*str) - '0');
				}
				if (*str == '.')
				{
					// Milliseconds may be given, we don't care.
					do
					{
						++str;
					} while (string::isNumberChar(*str));
				}
				if (*str != 'Z') // Time zone must not be present; we can't meaningfully deal with it without a more complex type and/or function signature.
				{
					return std::nullopt;
				}
				++str;
			}
		}
		if (*str != '\0')
		{
			return std::nullopt;
		}
		dt.setWdayFromDate();
		return dt;
	}

	std::time_t Datetime::toTimestamp() const
	{
		return time::toUnix(year, month, day, hour, minute, second);
	}

	std::string Datetime::toString() const
	{
		std::string str;
		str.append(string::lpad(std::to_string(this->hour), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(this->minute), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(this->second), 2, '0'));
		str.append(", ");
		str.append(std::to_string(this->day));
		if (this->month != 0)
		{
			str.push_back(' ');
			str.append(&months[(this->month - 1) * 3], 3);
		}
		return str;
	}

	// https://stackoverflow.com/a/66094245
	static int dayofweek(int d, int m, int y) noexcept
	{
		static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
		y -= m < 3;
		return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
	}

	void Datetime::setWdayFromDate() noexcept
	{
		wday = dayofweek(day, month, year);
	}

	Datetime time::datetimeUtc(std::time_t ts) noexcept
	{
#if SOUP_WINDOWS
		return Datetime::fromTm(*::gmtime(&ts));
#else
		struct tm t;
		::gmtime_r(&ts, &t);
		return Datetime::fromTm(t);
#endif
	}

	Datetime time::datetimeLocal(std::time_t ts) noexcept
	{
#if SOUP_WINDOWS
		return Datetime::fromTm(*::localtime(&ts));
#else
		struct tm t;
		::localtime_r(&ts, &t);
		return Datetime::fromTm(t);
#endif
	}

	int time::getLocalTimezoneOffset() noexcept
	{
		return datetimeLocal(0).hour - datetimeUtc(0).hour;
	}

	std::string time::toRfc2822(std::time_t ts)
	{
		const auto dt = datetimeUtc(ts);

		std::string str(&wdays[dt.wday * 3], 3);
		str.append(", ");
		str.append(std::to_string(dt.day));
		str.push_back(' ');
		str.append(&months[(dt.month - 1) * 3], 3);
		str.push_back(' ');
		str.append(std::to_string(dt.year));
		str.push_back(' ');
		str.append(string::lpad(std::to_string(dt.hour), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(dt.minute), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(dt.second), 2, '0'));
		str.append(" GMT");
		return str;
	}

	std::string time::toIso8601(std::time_t ts)
	{
		const auto dt = datetimeUtc(ts);

		std::string str = std::to_string(dt.year);
		str.push_back('-');
		str.append(string::lpad(std::to_string(dt.month), 2, '0'));
		str.push_back('-');
		str.append(string::lpad(std::to_string(dt.day), 2, '0'));
		str.push_back('T');
		str.append(string::lpad(std::to_string(dt.hour), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(dt.minute), 2, '0'));
		str.push_back(':');
		str.append(string::lpad(std::to_string(dt.second), 2, '0'));
		str.push_back('Z');
		return str;
	}

	std::time_t time::toUnix(int year, int month, int day, int hour, int minute, int second)
	{
		struct tm t;
		t.tm_year = year - 1900;
		t.tm_mon = month - 1;
		t.tm_mday = day;
		t.tm_hour = hour;
		t.tm_min = minute;
		t.tm_sec = second;
		return timegm(&t);
	}

	int time::isLeapYear(int year)
	{
		// We have a leap year if it is...
		return (year % 4) == 0 // divisible by 4 and either...
			&& (((year % 100) != 0) // not divisible by 100
				|| (year % 400) == 0 // or divisible by 400
				);
	}

	int time::getDaysInMonth(int year, int month)
	{
		switch (month)
		{
		case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
		case 2: return /*isLeapYear(year) ? 29 : 28*/ 28 + isLeapYear(year);
		case 4: case 6: case 9: case 11: return 30;
		}
		SOUP_ASSERT_UNREACHABLE;
	}
}
