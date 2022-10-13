#pragma once

#include <string>

#include "llex.h"

namespace Pluto // Decided to create the first instance of 'namespace' in this project. Figuring I'll put Pluto-related utility in here for now.
{
	struct Error
	{
	private:
		LexState* ls;
		size_t src_len = 0; // The size of the source line itself.
		size_t line_len = 0; // The buffer size needed to align a bar (|).

	public:
		std::string content{};

		Error(LexState* ls)
			: ls(ls)
		{
		}

		Error(LexState* ls, const std::string& initial_msg)
			: ls(ls), content(initial_msg)
		{
		}

		Error& addMsg(const std::string& msg)
		{
			this->content.append(msg);
			return *this;
		}

		Error& addSrcLine(int line)
		{
			const auto line_string = this->ls->getLineString(line);
			const auto init_len = this->content.length();
			this->content.append("\n    ");
			this->content.append(std::to_string(line));
			this->content.append(" | ");
			this->line_len = this->content.length() - init_len - 3;
			this->src_len += line_string.length();
			this->content.append(line_string);
			return *this;
		}

		Error& addGenericHere(const std::string& msg) // TO-DO: Add '^^^' strings for specific keywords. Not accurate with a simple string search.
		{
			this->content.push_back('\n');
			this->content.append(std::string(this->line_len, ' ') + "| ");
			this->content.append(std::string(this->src_len, '^'));
			this->content.append(" here: ");
			this->content.append(msg);
			return *this;
		}

		Error& addGenericHere()
		{
			this->content.push_back('\n');
			this->content.append(std::string(this->line_len, ' ') + "| ");
			this->content.append(std::string(this->src_len, '^'));
			this->content.append(" here");
			return *this;
		}

		Error& addNote(const std::string& msg)
		{
			const auto pad = std::string(this->line_len, ' ');
			this->content.push_back('\n');
			this->content.append(pad + "+ note: ");
			this->content.append(msg);
			return *this;
		}

		// Pushes the string to the stack for luaD_throw to conveniently pick up.
		void finalize()
		{
			lua_pushstring(ls->L, this->content.c_str());
		}
	};
}