#pragma once

#include <string>

#include "llex.h"
#include "vendor/Soup/soup/string.hpp"

namespace Pluto {
	class ErrorMessage {
	private:
		LexState* ls;
		size_t src_len = 0; // The size of the source line itself.
		size_t line_len = 0; // The buffer size needed to align a bar (|) or note (+).

	public:
		std::string content{};

		ErrorMessage(LexState* ls)
			: ls(ls) {
		}

		ErrorMessage(LexState* ls, const char* initial_msg)
			: ls(ls), content(initial_msg) {
		}

		ErrorMessage& addMsg(const char* msg) {
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

		ErrorMessage& addGenericHere(const char* msg) { // TO-DO: Add '^^^' strings for specific keywords. Not accurate with a simple string search.
#ifndef PLUTO_SHORT_ERRORS
			if (*msg == '\0') {
				return addGenericHere();
			}
			this->content.push_back('\n');
			this->content.append(this->line_len, ' ');
			this->content.append("| ");
			this->content.append(HBLU);
			this->content.append(this->src_len, '^');
			this->content.append(" here: ");
			this->content.append(msg);
			this->content.append(RESET);
#endif
			return *this;
		}

		ErrorMessage& addGenericHere() {
#ifndef PLUTO_SHORT_ERRORS
			this->content.push_back('\n');
			this->content.append(this->line_len, ' ');
			this->content.append("| ");
			this->content.append(HBLU);
			this->content.append(this->src_len, '^');
			this->content.append(" here");
			this->content.append(RESET);
#endif
			return *this;
		}

		ErrorMessage& addNote(std::string&& msg) {
#ifndef PLUTO_SHORT_ERRORS
			this->content.push_back('\n');
			this->content.append(this->line_len, ' ');
			this->content.append(HCYN "+ note: " RESET);

			if (msg.find("\n") != std::string::npos) { // Multi-line note?
				std::vector<std::string> lines = soup::string::explode(msg, '\n');
				
				if (lines.empty()) {
					this->content.append("There should be a note here, but something went wrong. Please report this at: https://github.com/PlutoLang/Pluto/issues");
				}
				else {
					this->content.append(lines[0]);

					for (auto i = lines.begin() + 1; i != lines.end(); ++i) {
						this->content.push_back('\n');
						this->content.append(this->line_len, ' ');
						this->content.append(sizeof("+ note: ") - 1, ' ');
						this->content.append(*i);
					}
				}
			}
			else {
				this->content.append(msg);
			}
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
			lua_State* L = ls->L;
			delete this;
			luaD_throw(L, LUA_ERRSYNTAX);
		}
	};
}