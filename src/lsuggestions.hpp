#pragma once

#include <utility>
#include <vector>

#include "llex.h"

struct SuggestionsState {
  LexState* const ls;

  const char* str = "";
  size_t len = 0;

  std::vector<std::pair<const char*, const char*>> suggestions;

  SuggestionsState(LexState* ls)
    : ls(ls) {
    if (ls->t.token != TK_SUGGEST_0) {
      luaX_next(ls); /* skip pluto_suggest_x */
      str = luaX_token2str_noq(ls, ls->t.token);
      len = strlen(str);
    }
    luaX_next(ls); /* skip pluto_suggest_0 or filter */
  }

  void push(const char* type, const char* name) {
    suggestions.emplace_back(type, name);
  }

  ~SuggestionsState() {
    /* filter suggestions */
    for (auto i = suggestions.begin(); i != suggestions.end(); ) {
      if (strncmp(i->second, str, len) == 0)
        ++i;
      else
        i = suggestions.erase(i);
    }

    /* emit suggestions if we have any */
    if (!suggestions.empty()) {
      std::string msg = "suggest: ";
      for (const auto& suggestion : suggestions) {
        msg.append(suggestion.first);
        msg.push_back(',');
        msg.append(suggestion.second);
        msg.push_back(';');
      }
      msg.pop_back();
      lua_warning(ls->L, msg.c_str(), 0);
    }
  }
};
