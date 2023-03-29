#pragma once

#include <string>

#include "llex.h"

namespace Pluto {
	class ErrorMessage {
	private:
		LexState* ls;
		size_t src_len = 0; // The size of the source line itself.
		size_t line_len = 0; // The buffer size needed to align a bar (|).

	public:
		std::string content{};

		ErrorMessage(LexState* ls)
			: ls(ls) {
		}

		ErrorMessage(LexState* ls, const std::string& initial_msg)
			: ls(ls), content(initial_msg) {
		}

		ErrorMessage& addMsg(const std::string& msg) {
			this->content.append(msg);
			return *this;
		}

		ErrorMessage& addSrcLine(int line) {
#ifndef PLUTO_SHORT_ERRORS
			const auto line_string = this->ls->getLineString(line);
			const auto init_len = this->content.length();
			this->content.append("\n    ");
			this->content.append(std::to_string(line));
			this->content.append(" | ");
			this->line_len = this->content.length() - init_len - 3;
			this->src_len += line_string.length();
			this->content.append(line_string);
#endif
			return *this;
		}

		ErrorMessage& addGenericHere(const std::string& msg) { // TO-DO: Add '^^^' strings for specific keywords. Not accurate with a simple string search.
#ifndef PLUTO_SHORT_ERRORS
			this->content.push_back('\n');
			this->content.append(std::string(this->line_len, ' ') + "| ");
			this->content.append(HBLU + std::string(this->src_len, '^'));
			this->content.append(" here: ");
			this->content.append(msg + RESET);
#endif
			return *this;
		}

		ErrorMessage& addGenericHere() {
#ifndef PLUTO_SHORT_ERRORS
			this->content.push_back('\n');
			this->content.append(std::string(this->line_len, ' ') + "| ");
			this->content.append(HBLU + std::string(this->src_len, '^'));
			this->content.append(" here");
			this->content.append(RESET);
#endif
			return *this;
		}

		ErrorMessage& addNote(const std::string& msg) {
#ifndef PLUTO_SHORT_ERRORS
			const auto pad = std::string(this->line_len, ' ');
			this->content.push_back('\n');
			this->content.append(pad + HCYN + "+ note: " + RESET);
			this->content.append(msg);
#endif
			return *this;
		}

		// Pushes the string to the stack for luaD_throw to conveniently pick up.
		void finalize() {
			this->content.append(RESET);
			lua_pushlstring(ls->L, this->content.data(), this->content.size());
		}

		[[noreturn]] void finalizeAndThrow() {
			this->finalize();
			luaD_throw(ls->L, LUA_ERRSYNTAX);
		}
	};
}