#pragma once

#include "base.hpp"
#include "fwd.hpp"

#include "Callback.hpp"
#include "Rgb.hpp"
#include "string.hpp"

#include <iostream>

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <termios.h>
#endif

#define BEL "\x7"
#define ESC "\x1B"
#define CSI ESC "["
#define OSC ESC "]"
#define ST  ESC "\\"

NAMESPACE_SOUP
{
	enum ControlInput : uint8_t
	{
		UP,
		DOWN,
		LEFT,
		RIGHT,
		NEW_LINE,
		BACKSPACE,
	};

	class console_impl
	{
	private:
#if SOUP_WINDOWS
		bool pressed_lmb = false;
		bool pressed_rmb = false;
		bool pressed_mmb = false;
#else
		inline static struct termios termattrs_og{};
		inline static struct termios termattrs_cur{};
#endif

	public:
		using char_handler_t = void(*)(char32_t);
		using control_handler_t = void(*)(ControlInput);

		EventHandler<void(char32_t)> char_handler;
		EventHandler<void(ControlInput)> control_handler;
	private:
		EventHandler<void(MouseButton, unsigned int, unsigned int)> mouse_click_handler;

	public:
		void init(bool fullscreen);
		void run();
		void cleanup();

		void onMouseClick(void(*fp)(MouseButton, unsigned int, unsigned int, const Capture&), Capture&& cap = {});

		/* This shit is only supported by MingW, like, why bother when you don't support anything else?!

		void hideScrollbar()
		{
			std::cout << CSI "?47h";
			std::cout << CSI "?30l";
		}

		void showScrollbar()
		{
			std::cout << CSI "?47l";
			std::cout << CSI "?30h";
		}*/

		// Window

		static void setTitle(const std::string& title);

		inline static EventHandler<void(unsigned int, unsigned int)> size_handler;
#if SOUP_POSIX
		static void sigwinch_handler_proc(int);
#endif
		static void enableSizeTracking(void(*fp)(unsigned int, unsigned int, const Capture&), Capture&& cap = {});

		// Output

		static void bell();
		static void clearScreen();
		static void hideCursor();
		static void showCursor();
		static void saveCursorPos();
		static void restoreCursorPos();
		static void fillScreen(Rgb c);
		static void fillScreen(unsigned int r, unsigned int g, unsigned int b);
		static void setCursorPos(unsigned int x, unsigned int y);

		template <typename T>
		const console_impl& operator << (const T& str) const
		{
			std::cout << str;
			return *this;
		}

		const console_impl& operator << (const std::u16string& str) const;

		static void setForegroundColour(Rgb c);

		static void setForegroundColour(int r, int g, int b);

		template <typename Str>
		[[nodiscard]] static Str strSetForegroundColour(int r, int g, int b)
		{
			Str str;
			str.push_back(CSI[0]);
			str.push_back(CSI[1]);
			str.push_back('3');
			str.push_back('8');
			str.push_back(';');
			str.push_back('2');
			str.push_back(';');
			str.append(string::decimal<Str>(r));
			str.push_back(';');
			str.append(string::decimal<Str>(g));
			str.push_back(';');
			str.append(string::decimal<Str>(b));
			str.push_back('m');
			return str;
		}

		static void setBackgroundColour(Rgb c);

		static void setBackgroundColour(int r, int g, int b);

		template <typename Str>
		[[nodiscard]] static Str strSetBackgroundColour(int r, int g, int b)
		{
			Str str;
			str.push_back(CSI[0]);
			str.push_back(CSI[1]);
			str.push_back('4');
			str.push_back('8');
			str.push_back(';');
			str.push_back('2');
			str.push_back(';');
			str.append(string::decimal<Str>(r));
			str.push_back(';');
			str.append(string::decimal<Str>(g));
			str.push_back(';');
			str.append(string::decimal<Str>(b));
			str.push_back('m');
			return str;
		}

		static void resetColour();

		template <typename Str>
		[[nodiscard]] static Str strResetColour()
		{
			Str str;
			str.push_back(CSI[0]);
			str.push_back(CSI[1]);
			str.push_back('m');
			return str;
		}

		// Ctrl+C

	private:
		using ctrl_c_handler_t = void(*)();

		inline static ctrl_c_handler_t ctrl_c_handler = nullptr;

#if SOUP_WINDOWS
		static BOOL WINAPI CtrlHandler(DWORD ctrlType);
#else
		static void sigint_handler_proc(int);
#endif

	public:
		static void overrideCtrlC(ctrl_c_handler_t handler);
	};

	inline console_impl console;
}

#undef BEL
#undef ESC
#undef CSI
#undef OSC
#undef ST
