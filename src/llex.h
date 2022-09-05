#pragma once
/*
** $Id: llex.h $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#include <limits.h>

#include <string>
#include <vector>
#include <string_view>

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
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_INLINE, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_PSWITCH, TK_PCASE, TK_PDEFAULT, TK_PCONTINUE, TK_PWHEN, // New compatibility keywords.
  /* New non-compatible keywords. */
#ifndef PLUTO_COMPATIBLE_SWITCH
  TK_SWITCH,
#endif
#ifndef PLUTO_COMPATIBLE_CASE
  TK_CASE,
#endif
#ifndef PLUTO_COMPATIBLE_DEFAULT
  TK_DEFAULT,
#endif
#ifndef PLUTO_COMPATIBLE_CONTINUE
  TK_CONTINUE,
#endif
#ifndef PLUTO_COMPATIBLE_WHEN
  TK_WHEN,
#endif
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  TK_IDIV, TK_CONCAT,
  TK_DOTS, TK_EQ,
  TK_GE, TK_LE,
  TK_NE, TK_SHL,
  TK_SHR, TK_DBCOLON, 
  TK_EOS, TK_FLT, 
  TK_INT, TK_NAME, TK_STRING,
  /* Pluto symbols */
  TK_CSUB, TK_CSHL,     /* subtraction & shift left    */
  TK_CSHR, TK_CBAND,    /* shift right & bitwise AND   */
  TK_CADD, TK_CMUL,     /* addition and multiplication */
  TK_CMOD, TK_CBOR,     /* modulo and bitwise OR       */
  TK_CBXOR,             /* bitwise XOR                 */
  TK_CIDIV, TK_CDIV,    /* integer and float division  */
  TK_CPOW, TK_POW,      /* exponents / power           */
  TK_CCAT, TK_COAL,     /* concatenation & null coal.  */
  TK_WALRUS,            /* walrus operator */
};

#define LAST_RESERVED TK_WHILE

/* number of reserved words */
#define NUM_RESERVED	(cast_int(LAST_RESERVED-FIRST_RESERVED + 1))


typedef union {
  lua_Number r;
  lua_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */


struct Token {
  int token;
  SemInfo seminfo;
  int line;

  /*
  ** This could be implemented using operator overloading.
  ** I dislike this approach because it's unnecessarily ambiguous. 
  ** I tend to avoid overloading as a whole, because it's typically unclear.
  **
  ** - Ryan
  */
  [[nodiscard]] bool Is(int t) const noexcept
  {
    return token == t;
  }

  [[nodiscard]] bool IsReserved() const noexcept
  {
    return token >= FIRST_RESERVED && token <= LAST_RESERVED;
  }

  /// Does this token escape control flow? I.e, a TK_BREAK or TK_CONTINUE?
  [[nodiscard]] bool IsEscapingToken() const noexcept
  {
    return token == TK_BREAK || token == TK_CONTINUE;
  }

  [[nodiscard]] bool IsReservedNonValue() const noexcept
  {
    return IsReserved() && token != TK_TRUE && token != TK_FALSE && token != TK_NIL;
  }
};


/*
** If you wish to add a new warning type, you need to update WarningType from the bottom.
** Then, you need to enter the 'name' of your warning at the bottom of luaX_warnIds, so the user can toggle it during runtime.
*/


enum WarningType : int
{
  ALL_WARNINGS = 0,

  VAR_SHADOW,
  TYPE_MISMATCH,
  UNREACHABLE_CODE,
  EXCESSIVE_ARGUMENTS,

  NUM_WARNING_TYPES
};


static const std::vector<std::string> luaX_warnNames = {
  "all",
  "var-shadow",
  "type-mismatch",
  "unreachable-code",
  "excessive-arguments",
};


struct WarningConfig
{
  bool toggles[NUM_WARNING_TYPES];

  WarningConfig()
  {
    setAllTo(true);
  }

  [[nodiscard]] bool& Get(WarningType type) noexcept
  {
    return toggles[type];
  }

  void setAllTo(bool newState) noexcept
  {
    for (int id = 0; id != NUM_WARNING_TYPES; ++id)
    {
      toggles[id] = newState;
    }
  }

  void processComment(const std::string& line) noexcept
  {
    for (int id = 0; id != NUM_WARNING_TYPES; ++id)
    {
      std::string enable  = "enable-";
      std::string disable = "disable-";

      const std::string& name = luaX_warnNames[id];

      enable += name;
      disable += name;

      if (line.find(enable) != std::string::npos)
      {
        if (name != "all")
          Get((WarningType)id) = true;
        else
          setAllTo(true);
      }
      else if (line.find(disable) != std::string::npos)
      {
        if (name != "all")
          Get((WarningType)id) = false;
        else
          setAllTo(false);
      }
    }
  }
};


/*
** State of the lexer plus state of the parser when shared by all functions.
** Suppression of C26495 (uninitalized member), because it's initialized elsewhere. 
*/
#if defined(_MSC_VER) && _MSC_VER && !__INTEL_COMPILER
#pragma warning( disable: 26495 )
#endif

struct LexState {
  int current;  /* current character (charint) */
  std::vector<std::string> lines;  /* A vector of all the lines processed by the lexer. */
  int lastline = 0;  /* line of last token 'consumed' */
  Token laststat;  /* the last statement */
  size_t tidx = -1;
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
  WarningConfig warning;  /* Configuration class for compile-time warnings. */
  bool creating_multiple_variables = false;
  bool processing_funcargs = false;
  void* inlinefunccall_bl = nullptr;

  LexState()
    : lines { std::string {} }
  {
    laststat = Token {};
    laststat.token = TK_EOS;
  }

  [[nodiscard]] int getLineNumber() const noexcept {
    return tidx == -1 ? 1 : tokens.at(tidx).line;
  }

  /// Find a substring within the current line buffer.
  /// There is an overload that allows a line number as the first argument to search a specific line.
  [[nodiscard]] bool findWithinLine(const std::string& substr, int offset = 0) noexcept
  {
    const std::string& str = getLineBuff();
    return str.find(substr, offset) != std::string::npos;
  }

  /// Find a substring within the respective line.
  /// There is an overload that allows you to omit the 'line' argument and default to the current line buffer.
  [[nodiscard]] bool findWithinLine(int line, const std::string& substr, int offset = 0) const noexcept {
    const std::string& str = getLineString(line);
    return str.find(substr, offset) != std::string::npos;
  }

  [[nodiscard]] int getLineNumberOfLastNonEmptyLine() const noexcept {
    for (int line = getLineNumber(); line != 0; --line) {
      if (!getLineString(line).empty()) {
        return line;
      }
    }
    return getLineNumber();
  }

  [[nodiscard]] const std::string& getLineString(int line) const {
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

  [[nodiscard]] bool isEvaluatingInlineFuncCall() const noexcept {
    return inlinefunccall_bl != nullptr;
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
LUAI_FUNC void luaX_setpos(LexState* ls, size_t pos);
LUAI_FUNC int luaX_lookahead(LexState *ls);
LUAI_FUNC const Token& luaX_lookbehind(LexState *ls);
[[noreturn]] LUAI_FUNC void luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);
LUAI_FUNC const char *luaX_token2str_noq (LexState *ls, int token);
LUAI_FUNC const char *luaX_reserved2str (int token);
