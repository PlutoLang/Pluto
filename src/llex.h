#pragma once
/*
** $Id: llex.h $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#include <limits.h>

#include <cstring> // memcpy
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "lobject.h"
#include "lzio.h"


/*
** Single-char tokens (terminal symbols) are represented by their own
** numeric code. Other tokens start at the following value.
*/
#define FIRST_RESERVED	(UCHAR_MAX + 1)


#if !defined(LUA_ENV)
#define LUA_ENV		"_ENV"
#endif


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_CASE, TK_DEFAULT, TK_AS, TK_BEGIN, TK_EXTENDS, TK_INSTANCEOF, // New narrow keywords.
  TK_PUSE, // New compatibility keywords.
  TK_PSWITCH, TK_PCONTINUE, TK_PENUM, TK_PNEW, TK_PCLASS, TK_PPARENT, TK_PEXPORT, TK_PTRY, TK_PCATCH,
  TK_SWITCH, TK_CONTINUE, TK_ENUM, TK_NEW, TK_CLASS, TK_PARENT, TK_EXPORT, TK_TRY, TK_CATCH, // New non-compatible keywords.
  TK_LET, TK_CONST, // New optional keywords.
  TK_SUGGEST_0, TK_SUGGEST_1, // New special keywords.
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  TK_IDIV, TK_CONCAT, TK_DOTS,
  TK_EQ, TK_GE, TK_LE, TK_NE, TK_SPACESHIP,
  TK_SHL, TK_SHR, TK_DBCOLON, TK_EOS,
  TK_FLT, TK_INT, TK_NAME, TK_STRING,
  /* Pluto symbols */
  TK_POW,     /* exponents / power */
  TK_COAL,    /* null coal.        */
  TK_WALRUS,  /* walrus operator   */
  TK_ARROW,
};

#define FIRST_COMPAT TK_PUSE
#define FIRST_NON_COMPAT TK_SWITCH
#define FIRST_OPTIONAL TK_LET
#define FIRST_SPECIAL TK_SUGGEST_0
#define LAST_RESERVED TK_WHILE

#define END_COMPAT FIRST_NON_COMPAT
#define END_NON_COMPAT FIRST_OPTIONAL

#define NUM_NON_COMPAT (END_NON_COMPAT - FIRST_NON_COMPAT)

/* number of reserved words */
#define NUM_RESERVED	(cast_int(LAST_RESERVED-FIRST_RESERVED + 1))


/* semantics information */
union SemInfo {
  lua_Number r;
  lua_Integer i;
  TString *ts;

  SemInfo() : ts(nullptr) {}
  SemInfo(TString *ts) : ts(ts) {}
  SemInfo(lua_Integer i) : i(i) {}
};


struct Token {
  int token;
  SemInfo seminfo;
  int line;

  Token() = default;

  // Can't be negative to avoid issues with precompiled code.
  static constexpr int LINE_INJECTED = 'plin';

  Token(int token)
    : token(token), line(LINE_INJECTED)
  {}

  Token(int token, TString* ts)
    : token(token), seminfo(ts), line(LINE_INJECTED)
  {}

  Token(int token, lua_Integer i)
    : token(token), seminfo(i), line(LINE_INJECTED)
  {}

  [[nodiscard]] bool Is(int t) const noexcept {
    return token == t;
  }

  [[nodiscard]] bool IsReserved() const noexcept {
    return token >= FIRST_RESERVED && token <= LAST_RESERVED;
  }

  [[nodiscard]] bool IsEscapingToken() const noexcept {
    return token == TK_BREAK || token == TK_CONTINUE;
  }

  [[nodiscard]] bool IsReservedNonValue() const noexcept {
    return IsReserved() && token != TK_TRUE && token != TK_FALSE && token != TK_NIL
      && token != TK_PPARENT
      && token != TK_PARENT
      ;
  }

  [[nodiscard]] bool IsNarrow() const noexcept {
    return token == TK_IN
      || (token >= TK_CASE && token < FIRST_COMPAT)
      ;
  }

  [[nodiscard]] bool IsCompatible() const noexcept {
      return (token >= FIRST_COMPAT && token < END_COMPAT);
  }

  [[nodiscard]] bool IsNonCompatible() const noexcept {
      return (token >= FIRST_NON_COMPAT && token < END_NON_COMPAT);
  }

  [[nodiscard]] bool IsOptional() const noexcept {
      return (token >= FIRST_OPTIONAL && token < FIRST_SPECIAL);
  }

  [[nodiscard]] bool IsSpecial() const noexcept {
      return (token >= FIRST_SPECIAL && token < TK_RETURN);
  }

  [[nodiscard]] bool IsOverridable() const noexcept {
      return token == TK_PARENT || token == TK_PPARENT;
  }
};


enum WarningType : int {
  ALL_WARNINGS = 0,

  WT_VAR_SHADOW,
  WT_GLOBAL_SHADOW,
  WT_TYPE_MISMATCH,
  WT_UNREACHABLE_CODE,
  WT_EXCESSIVE_ARGUMENTS,
  WT_DEPRECATED,
  WT_BAD_PRACTICE,
  WT_POSSIBLE_TYPO,
  WT_NON_PORTABLE_CODE,
  WT_NON_PORTABLE_BYTECODE,
  WT_NON_PORTABLE_NAME,

  NUM_WARNING_TYPES
};


inline const char* const luaX_warnNames[] = {
  "all",
  "var-shadow",
  "global-shadow",
  "type-mismatch",
  "unreachable-code",
  "excessive-arguments",
  "deprecated",
  "bad-practice",
  "possible-typo",
  "non-portable-code",
  "non-portable-bytecode",
  "non-portable-name",
};
static_assert(sizeof(luaX_warnNames) / sizeof(const char*) == NUM_WARNING_TYPES);


enum WarningState : lu_byte {
  WS_OFF,
  WS_ON,
  WS_ERROR,
};


class WarningConfig
{
public:
  const size_t begins_at;
  WarningState states[NUM_WARNING_TYPES];

private:
  [[nodiscard]] static WarningState getDefaultState(WarningType type) noexcept {
    switch (type) {
#ifndef PLUTO_WARN_NON_PORTABLE_CODE
    case WT_NON_PORTABLE_CODE:
#endif
#ifndef PLUTO_WARN_NON_PORTABLE_BYTECODE
    case WT_NON_PORTABLE_BYTECODE:
#endif
#ifndef PLUTO_WARN_NON_PORTABLE_NAME
    case WT_NON_PORTABLE_NAME:
#endif
    case NUM_WARNING_TYPES:  /* dummy case so compiler doesn't cry when all macros are set */
      return WS_OFF;
    default:
      return WS_ON;
    }
  }

public:
  WarningConfig(size_t begins_at) noexcept : begins_at(begins_at) {
    for (int id = 0; id != NUM_WARNING_TYPES; ++id) {
      states[id] = getDefaultState((WarningType)id);
    }
  }

  void copyFrom(const WarningConfig& b) noexcept {
    memcpy(states, b.states, sizeof(states));
  }

  [[nodiscard]] bool isEnabled(WarningType type) const noexcept {
    return states[type] != WS_OFF;
  }

  [[nodiscard]] bool isFatal(WarningType type) const noexcept {
    return states[type] == WS_ERROR;
  }

  void setAllTo(WarningState newState) noexcept {
    for (int id = 0; id != NUM_WARNING_TYPES; ++id) {
      states[id] = newState;
    }
  }

  void processComment(const std::string& line) noexcept {
    for (int id = 0; id != NUM_WARNING_TYPES; ++id) {
      const std::string& name = luaX_warnNames[id];
      if (line.find(name) == std::string::npos)
        continue;

      std::string enable = "enable-";
      std::string disable = "disable-";
      std::string error = "error-";

      enable += name;
      disable += name;
      error += name;

      if (line.find(enable) != std::string::npos) {
        if (name != "all")
          states[id] = WS_ON;
        else
          setAllTo(WS_ON);
      } else if (line.find(disable) != std::string::npos) {
        if (name != "all")
          states[id] = WS_OFF;
        else
          setAllTo(WS_OFF);
      } else if (line.find(error) != std::string::npos) {
        if (name != "all")
          states[id] = WS_ERROR;
        else
          setAllTo(WS_ERROR);
      }
    }
  }

  [[nodiscard]] static const char* getWarningName(const WarningType w) {
    lua_assert((size_t)w >= 0 && (size_t)w < NUM_WARNING_TYPES);
    return luaX_warnNames[(size_t)w];
  }
};


/*
** State of the lexer plus state of the parser when shared by all functions.
** Suppression of C26495 (uninitalized member), because it's initialized elsewhere. 
*/
#if defined(_MSC_VER) && _MSC_VER && !__INTEL_COMPILER
#pragma warning( disable: 26495 )
#endif

enum ParserContext : lu_byte {
  PARCTX_NONE,
  PARCTX_CREATE_VAR,
  PARCTX_CREATE_VARS,
  PARCTX_FUNCARGS,
  PARCTX_BODY,
};

struct ClassData {
  size_t parent_name_pos = 0;
  std::vector<std::string> private_fields{};
  
  [[nodiscard]] bool isPrivate(const char* fieldname) const noexcept {
    for (const auto& pf : private_fields) {
      if (pf == fieldname)
        return true;
    }
    return false;
  }
};

struct EnumDesc {
  struct Enumerator {
    TString* name;
    lua_Integer value;
  };
  std::vector<Enumerator> enumerators;
};

enum KeywordState : lu_byte {
  KS_ENABLED_BY_ENV,
  KS_DISABLED_BY_ENV,
  KS_ENABLED_BY_USER,
  KS_DISABLED_BY_USER,
};

struct LexState {
  int current;  /* current character (charint) */
  std::vector<std::string> lines;  /* A vector of all the lines processed by the lexer. */
  int lastline = 0;  /* line of last token 'consumed' */
  Token laststat;  /* the last statement */
  size_t tidx = -1;  /* [Pluto] token index of the parser, -1 during lexer pass */
  std::vector<Token> tokens;
  Token t;  /* current token */
  struct FuncState *fs;  /* current function (parser) */
  struct lua_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  Table *h;  /* to avoid collection/reuse strings */
  struct Dyndata *dyd;  /* dynamic structures used by the parser */
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */

  bool uses_new = false;
  bool uses_extends = false;
  bool uses_instanceof = false;
  bool uses_spaceship = false;

  std::vector<WarningConfig> warnconfs;
  std::stack<ParserContext> parser_context_stck{};
  std::stack<ClassData> classes{};
  std::vector<EnumDesc> enums{};
  std::vector<TString*> export_symbols{};
  std::vector<void*> parse_time_allocations{};
  std::unordered_map<const TString*, void*> global_props{};
  KeywordState keyword_states[NUM_NON_COMPAT];

  LexState() : lines{ std::string{} }, warnconfs{ WarningConfig(0) } {
    laststat = Token {};
    laststat.token = TK_EOS;
    parser_context_stck.push(PARCTX_NONE); /* ensure there is at least 1 item on the parser context stack */
    for (auto& ks : keyword_states) {
      ks = KS_ENABLED_BY_ENV;
    }
  }

  ~LexState() {
    for (auto& a : parse_time_allocations) {
      free(a);
    }
  }

  [[nodiscard]] bool hasDoneLexerPass() const noexcept {
    return !tokens.empty() && tokens.back().token == TK_EOS;
  }

  void setLineNumber(int l) {
    tidx = 0;
    for (auto& tk : tokens) {
      if (tk.line == l) {
        break;
      }
      ++tidx;
    }
  }

  [[nodiscard]] int getLineNumber() const noexcept {
    if (!tokens.empty() && tokens.back().token != TK_EOS) {  /* doing lexer pass? */
      return tokens.back().line;
    }
    return tidx == (size_t)-1 ? 1 : tokens.at(tidx).line;
  }

  [[nodiscard]] int getLineNumberOfLastNonEmptyLine() const noexcept {
    for (int line = getLineNumber(); line != 0; --line) {
      if (!getLineString(line).empty()) {
        return line;
      }
    }
    return getLineNumber();
  }

  inline static std::string injected_code_str = "[injected code]";

  [[nodiscard]] const std::string& getLineString(int line) const {
    if (line == Token::LINE_INJECTED)
      return injected_code_str;
    return lines.at(line - 1);
  }

  [[nodiscard]] std::string& getLineBuff() {
    return lines.back();
  }

  void appendLineBuff(const std::string& str) {
    getLineBuff().append(str);
  }

  void appendLineBuff(char c) {
    getLineBuff().push_back(c);
  }

  [[nodiscard]] ParserContext getContext() const noexcept {
    return parser_context_stck.top();
  }

  void pushContext(ParserContext ctx) {
    parser_context_stck.push(ctx);
  }

  void popContext(ParserContext ctx);

  [[nodiscard]] size_t getParentClassPos() const noexcept {
    if (classes.empty())
      return 0;
    return classes.top().parent_name_pos;
  }

  WarningConfig& lexPushWarningOverride() {
    if (warnconfs.back().begins_at == tokens.size()) {
      return warnconfs.back();
    }
    WarningConfig warnconf(tokens.size());
    warnconf.copyFrom(warnconfs.back());
    return warnconfs.emplace_back(std::move(warnconf));
  }

  [[nodiscard]] const WarningConfig& getWarningConfig() const noexcept {
    return getWarningConfig(tidx);
  }

  [[nodiscard]] const WarningConfig& getWarningConfig(size_t tidx) const noexcept {
    const WarningConfig* last = &warnconfs.at(0);
    for (const auto& warnconf : warnconfs) {
      if (warnconf.begins_at > tidx)
        break;
      last = &warnconf;
    }
    return *last;
  }

  [[nodiscard]] bool shouldEmitWarning(int line, WarningType warning_type) const {
    const auto& linebuff = this->getLineString(line);
    const auto& lastattr = line > 1 ? this->getLineString(line - 1) : linebuff;
    return lastattr.find("@pluto_warnings: disable-next") == std::string::npos && getWarningConfig().isEnabled(warning_type);
  }

  [[nodiscard]] bool shouldSuggest() const noexcept {
    return t.token == TK_SUGGEST_0 || t.token == TK_SUGGEST_1;
  }

  [[nodiscard]] static bool hasKeywordState(int t) noexcept {
    return t >= FIRST_NON_COMPAT && t < END_NON_COMPAT;
  }

  [[nodiscard]] bool isKeywordEnabled(int t) const noexcept {
    static_assert((KS_ENABLED_BY_USER & 1) == 0);
    static_assert((KS_ENABLED_BY_ENV & 1) == 0);
    static_assert((KS_DISABLED_BY_USER & 1) != 0);
    static_assert((KS_DISABLED_BY_ENV & 1) != 0);
    return (getKeywordState(t) & 1) == 0;
  }

  [[nodiscard]] KeywordState getKeywordState(int t) const noexcept {
    lua_assert(hasKeywordState(t));
    return keyword_states[t - FIRST_NON_COMPAT];
  }

  void setKeywordState(int t, KeywordState ks) noexcept {
    if (hasKeywordState(t))
      keyword_states[t - FIRST_NON_COMPAT] = ks;
  }
};

#if defined(_MSC_VER) && _MSC_VER && !__INTEL_COMPILER
#pragma warning( default: 26495 )
#endif


LUAI_FUNC void luaX_init (lua_State *L);
LUAI_FUNC void luaX_setinput (lua_State *L, LexState *ls, ZIO *z,
                              TString *source, int firstchar);
LUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC TString* luaX_newstring (LexState *ls, const char *str);
LUAI_FUNC void luaX_next (LexState *ls);
LUAI_FUNC void luaX_prev (LexState *ls);
[[nodiscard]] LUAI_FUNC size_t luaX_getpos(LexState *ls);
LUAI_FUNC void luaX_setpos(LexState *ls, size_t pos);
LUAI_FUNC int luaX_lookahead(LexState *ls);
LUAI_FUNC const Token& luaX_lookbehind(LexState *ls);
LUAI_FUNC l_noret luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);
LUAI_FUNC const char *luaX_token2str_noq (LexState *ls, int token);
LUAI_FUNC const char *luaX_reserved2str (int token);
LUAI_FUNC void luaX_checkspecial (LexState *ls);
