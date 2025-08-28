#pragma once

#include <string>

#include "llex.h"
#include "vendor/Soup/soup/string.hpp"

namespace Pluto {
	class ErrorMessage {
	private:
		LexState* ls;
		size_t src_off = 0;
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

		[[nodiscard]] static intptr_t encodePos(size_t tidx) {
			return ~tidx;
		}

		[[nodiscard]] static int decodeLine(LexState* ls, intptr_t line_or_tidx) {
			if (line_or_tidx >= 0) {
				return static_cast<int>(line_or_tidx);
			}
			size_t tidx = ~line_or_tidx;
			return ls->tokens.at(tidx).line;
		}

		ErrorMessage& addSrcLine(intptr_t line_or_tidx) {
#ifndef PLUTO_SHORT_ERRORS
			const auto line = decodeLine(ls, line_or_tidx);

			const auto init_len = this->content.length();
			this->content.append("\n    ");
			this->content.append(std::to_string(line));
			this->content.append(" | ");
			this->line_len = this->content.length() - init_len - 3;

			auto line_string = this->ls->getLineString(line);
			if (line_string.length() > 80) {
				if (auto sep = line_string.find("--"); sep != std::string::npos) {
					line_string.erase(sep);
					while (!line_string.empty() && line_string.back() == ' ') {
						line_string.pop_back();
					}
				}
			}
			this->src_len = line_string.length();
			this->content.append(line_string);

			if (line_or_tidx < 0) {
				const size_t tidx = ~line_or_tidx;
				const auto& tk = ls->tokens.at(tidx);
				if (tk.line == line && tk.column < this->src_len) {
					this->src_off = tk.column;
					const auto& ntk = ls->tokens.at(tidx + 1);
					if (ntk.line == line) {
						int eot = ntk.column;
						while (eot-- != 0 && soup::string::isSpace(line_string[eot]));
						++eot;
						if (eot > tk.column) {
							this->src_len = eot - tk.column;
						}
					}
					else {
						this->src_len -= src_off;
					}
				}
			}
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
			this->content.append(this->src_off, ' ');
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
			this->content.append(this->src_off, ' ');
			this->content.append(this->src_len, '^');
			this->content.append(" here");
			this->content.append(RESET);
#endif
			return *this;
		}

		ErrorMessage& addNote(const char* msg) {
#ifndef PLUTO_SHORT_ERRORS
			if (*msg != '\0') {
				this->content.push_back('\n');
				this->content.append(this->line_len, ' ');
				this->content.append(HCYN "+ note: " RESET);

				if (strchr(msg, '\n') != nullptr) { // Multi-line note?
					std::vector<std::string> lines = soup::string::explode<std::string>(msg, '\n');

					lua_assert(!lines.empty());

					this->content.append(lines[0]);

					for (auto i = lines.begin() + 1; i != lines.end(); ++i) {
						this->content.push_back('\n');
						this->content.append(this->line_len, ' ');
						this->content.append(sizeof("+ note: ") - 1, ' ');
						this->content.append(*i);
					}
				}
				else {
					this->content.append(msg);
				}
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