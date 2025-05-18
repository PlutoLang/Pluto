#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct Datetime
	{
		int year;
		int month; // 1-12
		int day; // 1-31
		int hour;
		int minute;
		int second;

		int wday; // days since sunday (0-6)

		[[nodiscard]] static Datetime fromTm(const struct tm& t) noexcept;
		[[nodiscard]] static std::optional<Datetime> fromIso8601(const char* str) noexcept;

		[[nodiscard]] std::time_t toTimestamp() const;
		[[nodiscard]] std::string toString() const; // example: "00:00:00, 1 Jan"

		void setWdayFromDate() noexcept;
	};

	struct time
	{
		// When using time::millisSince(0), the return value might be rather small if the system epoch is recent.
		// Using time::LONGAGO instead of 0 will ensure that the "millisSince" will be sufficiently large.
		static constexpr std::time_t LONGAGO = -0x1000'0000;

		// System-dependent time

		[[nodiscard]] static std::time_t millis() noexcept
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		[[nodiscard]] static std::time_t millisSince(std::time_t since) noexcept
		{
			return millis() - since;
		}

		[[nodiscard]] static std::time_t millisUntil(std::time_t until) noexcept
		{
			return until - millis();
		}

		[[nodiscard]] static std::time_t micros() noexcept
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		[[nodiscard]] static std::time_t nanos() noexcept
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		// UNIX time (seconds since 00:00, Jan 1, 1970 UTC)

		[[nodiscard]] static std::time_t unixSeconds() noexcept
		{
			return ::time(nullptr); // <-- ~0.0000006ms
			//                           v ~0.000002ms
			//return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		}

		[[nodiscard]] static std::time_t unixSecondsSince(std::time_t since) noexcept
		{
			return unixSeconds() - since;
		}

		[[nodiscard]] static std::time_t unixSecondsUntil(std::time_t until) noexcept
		{
			return until - unixSeconds();
		}

		[[nodiscard]] static Datetime datetimeUtc(std::time_t ts) noexcept; // construct datetime based on UNIX time stamp
		[[nodiscard]] static Datetime datetimeLocal(std::time_t ts) noexcept; // construct datetime based on local time at UNIX time stamp

		[[nodiscard]] static int getLocalTimezoneOffset() noexcept;

		[[nodiscard]] static std::string toRfc2822(std::time_t ts); // construct date and time string as defined in RFC 2822 (ex: "Thu, 1 Jan 1970 00:00:00 GMT") based on UNIX time stamp
		[[nodiscard]] static std::string toIso8601(std::time_t ts); // construct date and time string as defined in ISO 8601 (ex: "1970-01-01T00:00:00Z") based on UNIX time stamp

		[[nodiscard]] static std::time_t toUnix(int year, int month, int day, int hour, int minute, int second);

		[[nodiscard]] static std::time_t unixFromFile(std::filesystem::file_time_type ft)
		{
			return std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
				).time_since_epoch()
			).count();
		}

		[[nodiscard]] static std::filesystem::file_time_type fileFromUnix(std::time_t ut)
		{
			std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(ut);
			return tp - std::chrono::system_clock::now() + std::filesystem::file_time_type::clock::now();
		}

		// Misc

		[[nodiscard]] static int isLeapYear(int year);
		[[nodiscard]] static int getDaysInMonth(int year, int month); // month starts at 1
	};
}
