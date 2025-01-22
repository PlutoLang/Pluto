#include "os.hpp"

#include <array>
#include <cstring> // memcpy
#include <fstream>

#if SOUP_WINDOWS
	#pragma comment(lib, "Gdi32.lib")

	#include <psapi.h>

	#include "Exception.hpp"
	#include "ObfusString.hpp"
#else
	#include <unistd.h> // getpid, usleep
	#if _POSIX_C_SOURCE >= 199309L
		#include <time.h> // nanosleep
	#endif
#endif

#include "filesystem.hpp"
#include "rand.hpp"
#include "string.hpp"
#include "unicode.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	void os::escape(std::string& str)
	{
		if (str.find(' ') != std::string::npos)
		{
			escapeNoCheck(str);
		}
	}

	void os::escapeNoCheck(std::string& str)
	{
		string::replaceAll(str, "\\", "\\\\");
		string::replaceAll(str, "\"", "\\\"");
		str.insert(0, 1, '"');
		str.push_back('"');
	}

	std::string os::execute(std::string program, const std::vector<std::string>& args)
	{
		resolveProgram(program);
		return executeInner(std::move(program), args);
	}

	std::string os::executeLong(std::string program, const std::vector<std::string>& args)
	{
		resolveProgram(program);
		std::string flatargs;
		for (auto i = args.begin(); i != args.end(); ++i)
		{
			std::string escaped = *i;
			escapeNoCheck(escaped);
			if (!flatargs.empty())
			{
				flatargs.push_back(' ');
			}
			flatargs.append(escaped);
		}
		auto args_file = filesystem::tempfile();
		{
			std::ofstream argsof(args_file);
			argsof << std::move(flatargs);
		}
		auto ret = executeInner(std::move(program), { std::move(std::string(1, '@').append(args_file.string())) });
		std::error_code ec;
		std::filesystem::remove(args_file, ec);
		return ret;
	}

	void os::resolveProgram(std::string& program)
	{
#if SOUP_WINDOWS
		if (program.find('\\') == std::string::npos
			&& program.find('/') == std::string::npos
			)
		{
			std::string program_og = program;
			program = executeInner("where", { program });
			if (program.substr(0, 6) == "INFO: ")
			{
				std::string msg = "Failed to find program \"";
				msg.append(program_og);
				msg.push_back('"');
				SOUP_THROW(Exception(std::move(msg)));
			}
			string::limit<std::string>(program, "\n");
		}
#endif
	}

	std::string os::executeInner(std::string cmd, const std::vector<std::string>& args)
	{
		escape(cmd);
		for (auto i = args.begin(); i != args.end(); ++i)
		{
			std::string escaped = *i;
			escape(escaped);
			cmd.push_back(' ');
			cmd.append(escaped);
		}
		cmd.append(" 2>&1");
		//std::cout << "Running <" << cmd << ">" << std::endl;
#if SOUP_WINDOWS
		auto pipe = _popen(cmd.c_str(), "r");
#else
		auto pipe = popen(cmd.c_str(), "r");
#endif
		std::array<char, 128> buffer;
		std::string result;
		while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		{
			result += buffer.data();
		}
#if SOUP_WINDOWS
		_pclose(pipe);
#else
		pclose(pipe);
#endif
		return result;
	}

#if !SOUP_WINDOWS
	pid_t os::getProcessId() noexcept
	{
		return ::getpid();
	}

	void os::sleep(unsigned int ms) noexcept
	{
#if _POSIX_C_SOURCE >= 199309L
		struct timespec ts;
		ts.tv_sec = ms / 1000;
		ts.tv_nsec = (ms % 1000) * 1000000;
		int res;
		do
		{
			res = ::nanosleep(&ts, &ts);
		} while (res && errno == EINTR);
#else
		if (ms >= 1000)
		{
			::sleep(ms / 1000);
		}
		::usleep((ms % 1000) * 1000);
#endif
	}
#endif

#if SOUP_WINDOWS
	static bool copy_to_clipboard_utf16(const std::wstring& text)
	{
		const size_t len = (text.length() + 1) * sizeof(wchar_t);
		HGLOBAL hMem = GlobalAlloc(GHND | GMEM_DDESHARE, len);
		if (hMem != nullptr)
		{
			void* pMem = GlobalLock(hMem);
			if (pMem != nullptr)
			{
				memcpy(pMem, text.data(), len);
				GlobalUnlock(hMem);
				if (OpenClipboard(nullptr))
				{
					bool success = (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, hMem) != nullptr);
					CloseClipboard();
					return success;
				}
				GlobalFree(hMem);
			}
		}
		return false;
	}

	bool os::copyToClipboard(const std::string& text)
	{
		return copy_to_clipboard_utf16(unicode::utf8_to_utf16(text));
	}

	std::string os::getClipboardTextUtf8()
	{
		return unicode::utf16_to_utf8(getClipboardTextUtf16());
	}

	UTF16_STRING_TYPE os::getClipboardTextUtf16()
	{
		std::wstring text;
		if (OpenClipboard(nullptr))
		{
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData != nullptr)
			{
				auto pszText = static_cast<wchar_t*>(GlobalLock(hData));
				if (pszText != nullptr)
				{
					text = pszText;
					GlobalUnlock(hData);
				}
			}
			CloseClipboard();
		}
		return text;
	}

	#if !SOUP_CROSS_COMPILE
	size_t os::getMemoryUsage()
	{
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		return pmc.PrivateUsage;
	}
	#endif

	bool os::isWine()
	{
		return GetProcAddress(LoadLibraryA(ObfusString("ntdll.dll")), ObfusString("wine_get_version")) != nullptr;
	}

	PEB* os::getCurrentPeb()
	{
		// There is a "simpler" solution (https://gist.github.com/Wack0/849348f9d4f3a73dac864a556e9372a5), but this is what Microsoft does, so we shall, too.

		auto ntdll = LoadLibraryA(ObfusString("ntdll.dll"));

		using NtQueryInformationProcess_t = NTSTATUS(NTAPI*)(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength);
		auto NtQueryInformationProcess = (NtQueryInformationProcess_t)GetProcAddress(ntdll, ObfusString("NtQueryInformationProcess"));

		PROCESS_BASIC_INFORMATION ProcessInformation;
		NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &ProcessInformation, sizeof(ProcessInformation), 0);

		return ProcessInformation.PebBaseAddress;
	}

	#if !SOUP_CROSS_COMPILE
	[[nodiscard]] static std::string HBMITMAP_to_BMP(HBITMAP hBitmap)
	{
		HDC hDC;
		int iBits;
		WORD wBitCount;
		DWORD dwPaletteSize = 0, dwBmBitsSize = 0, dwDIBSize = 0;
		BITMAP Bitmap0;
		BITMAPFILEHEADER bmfHdr;
		BITMAPINFOHEADER bi;
		LPBITMAPINFOHEADER lpbi;
		HANDLE hDib, hPal, hOldPal2 = NULL;
		hDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
		iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
		DeleteDC(hDC);
		if (iBits <= 1)
			wBitCount = 1;
		else if (iBits <= 4)
			wBitCount = 4;
		else if (iBits <= 8)
			wBitCount = 8;
		else
			wBitCount = 24;
		GetObject(hBitmap, sizeof(Bitmap0), (LPSTR)&Bitmap0);
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = Bitmap0.bmWidth;
		bi.biHeight = -Bitmap0.bmHeight;
		bi.biPlanes = 1;
		bi.biBitCount = wBitCount;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrImportant = 0;
		bi.biClrUsed = 256;
		dwBmBitsSize = ((Bitmap0.bmWidth * wBitCount + 31) & ~31) / 8
			* Bitmap0.bmHeight;
		hDib = GlobalAlloc(GHND, dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
		lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
		*lpbi = bi;

		hPal = GetStockObject(DEFAULT_PALETTE);
		if (hPal)
		{
			hDC = GetDC(NULL);
			hOldPal2 = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
			RealizePalette(hDC);
		}


		GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap0.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER)
			+ dwPaletteSize, (BITMAPINFO*)lpbi, DIB_RGB_COLORS);

		if (hOldPal2)
		{
			SelectPalette(hDC, (HPALETTE)hOldPal2, TRUE);
			RealizePalette(hDC);
			ReleaseDC(NULL, hDC);
		}

		bmfHdr.bfType = 0x4D42; // "BM"
		dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
		bmfHdr.bfSize = dwDIBSize;
		bmfHdr.bfReserved1 = 0;
		bmfHdr.bfReserved2 = 0;
		bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

		std::string data;
		data.reserve(sizeof(BITMAPFILEHEADER) + dwDIBSize);
		data.append((const char*)&bmfHdr, sizeof(BITMAPFILEHEADER));
		data.append((const char*)lpbi, dwDIBSize);

		GlobalUnlock(hDib);
		GlobalFree(hDib);

		return data;
	}

	std::string os::makeScreenshotBmp(int x, int y, int width, int height)
	{
		HDC dcScreen = GetDC(0);
		HDC dcTarget = CreateCompatibleDC(dcScreen);
		HBITMAP bmpTarget = CreateCompatibleBitmap(dcScreen, width, height);
		HGDIOBJ oldBmp = SelectObject(dcTarget, bmpTarget);
		BitBlt(dcTarget, 0, 0, width, height, dcScreen, x, y, SRCCOPY | CAPTUREBLT);
		SelectObject(dcTarget, oldBmp);
		DeleteDC(dcTarget);
		ReleaseDC(0, dcScreen);

		auto bmp = HBMITMAP_to_BMP(bmpTarget);
		DeleteObject(bmpTarget);
		return bmp;
	}
	#endif
#endif
}
