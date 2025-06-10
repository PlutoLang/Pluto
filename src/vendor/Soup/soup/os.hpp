#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "type.hpp"

#include <string>
#include <vector>

#if SOUP_WINDOWS
#include <windows.h>
#include <winternl.h>
#endif

NAMESPACE_SOUP
{
	class os
	{
	public:
		static void escape(std::string& str);
	private:
		static void escapeNoCheck(std::string& str);
	public:
		static std::string execute(std::string program, const std::vector<std::string>& args = {});
		static std::string executeLong(std::string program, const std::vector<std::string>& args = {});
	private:
		static void resolveProgram(std::string& program);
		static std::string executeInner(std::string program, const std::vector<std::string>& args);
	public:

		[[nodiscard]] static pid_t getProcessId() noexcept;

		static void sleep(unsigned int ms) noexcept;
		static void fastSleep(unsigned int ms) noexcept; // On Windows, this tries to be more accurate for ms < 15.

#if SOUP_WINDOWS
		static bool copyToClipboard(const std::string& text);
		[[nodiscard]] static std::string getClipboardTextUtf8();
		[[nodiscard]] static std::wstring getClipboardTextUtf16();

	#if !SOUP_CROSS_COMPILE
		[[nodiscard]] static size_t getMemoryUsage();
	#endif

		[[nodiscard]] static bool isWine();

		[[nodiscard]] static PEB* getCurrentPeb();

	#if !SOUP_CROSS_COMPILE
		[[nodiscard]] static std::string makeScreenshotBmp(int x, int y, int width, int height);
	#endif

		[[nodiscard]] static int getPrimaryScreenWidth()
		{
			return GetSystemMetrics(SM_CXSCREEN);
		}

		[[nodiscard]] static int getPrimaryScreenHeight()
		{
			return GetSystemMetrics(SM_CYSCREEN);
		}
#endif
	};

#if SOUP_WINDOWS
	inline pid_t os::getProcessId() noexcept
	{
		return GetCurrentProcessId();
	}

	inline void os::sleep(unsigned int ms) noexcept
	{
		::Sleep(ms);
	}
#else
	inline void os::fastSleep(unsigned int ms) noexcept
	{
		os::sleep(ms);
	}
#endif
}
