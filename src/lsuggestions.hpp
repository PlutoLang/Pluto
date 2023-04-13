#pragma once

#include <string>
#include <utility>
#include <vector>

#include "llex.h"

struct SuggestionsState {
  struct Suggestion {
    const char* type;
    const char* name;
    std::string extra;
  };

  LexState* const ls;

  const char* str = "";
  size_t len = 0;

  std::vector<Suggestion> suggestions;

  SuggestionsState(LexState* ls)
    : ls(ls) {
    if (ls->t.token != TK_SUGGEST_0) {
      luaX_next(ls); /* skip pluto_suggest_x */
      str = luaX_token2str_noq(ls, ls->t.token);
      len = strlen(str);
    }
    luaX_next(ls); /* skip pluto_suggest_0 or filter */
  }

  void push(const char* type, const char* name, std::string extra = {}) {
    suggestions.emplace_back(Suggestion{ type, name, std::move(extra) });
  }

  void pushLocals() {
    for (int i = ls->fs->nactvar - 1; i >= 0; i--) {
      Vardesc *vd = getlocalvardesc(ls->fs, i);
      if (strcmp(vd->vd.name->contents, "Pluto_operator_new") != 0
          && strcmp(vd->vd.name->contents, "Pluto_operator_extends") != 0
          && strcmp(vd->vd.name->contents, "Pluto_operator_instanceof") != 0
        ) {
        push("local", vd->vd.name->contents);
      }
    }
  }

  ~SuggestionsState() {
    /* filter suggestions */
    for (auto i = suggestions.begin(); i != suggestions.end(); ) {
      if (strncmp(i->name, str, len) == 0)
        ++i;
      else
        i = suggestions.erase(i);
    }

    /* emit suggestions if we have any */
    if (!suggestions.empty()) {
      std::string msg = "suggest: ";
      for (const auto& suggestion : suggestions) {
        msg.append(suggestion.type);
        msg.push_back(',');
        msg.append(suggestion.name);
        if (!suggestion.extra.empty()) {
          msg.push_back(',');
          msg.append(suggestion.extra);
        }
        msg.push_back(';');
      }
      msg.pop_back();
      lua_warning(ls->L, msg.c_str(), 0);
    }
  }
};
