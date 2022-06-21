/*
** $Id: llex.h $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#ifndef llex_h
#define llex_h

#include <string>
#include <limits.h>

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
  TK_AND = FIRST_RESERVED, TK_BREAK, TK_CONTINUE,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  // TK_SWITCH, TK_CASE, TK_DEFAULT,  // Switch statement
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
  TK_CBXOR, TK_CBNOT,   /* bitwise XOR and bitwise NOT */
  TK_CIDIV, TK_CDIV,    /* integer and float division  */
  TK_CPOW, TK_POW       /* exponents / power           */
};

/* number of reserved words */
#define NUM_RESERVED	(cast_int(TK_WHILE-FIRST_RESERVED + 1))


typedef union {
  lua_Number r;
  lua_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int token;
  SemInfo seminfo;
} Token;


/* state of the lexer plus state of the parser when shared by all
   functions */
typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  int lasttoken;  /* save the last compound binary operator, if exists */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  struct FuncState *fs;  /* current function (parser) */
  struct lua_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  std::string linebuff; /* buffer for lines */
  std::string lastlinebuff; /* buffer for the last line */
  int lastlinebuffnum; /* last line number for lastlinebuff */
  Table *h;  /* to avoid collection/reuse strings */
  struct Dyndata *dyd;  /* dynamic structures used by the parser */
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */
} LexState;


LUAI_FUNC void luaX_init (lua_State *L);
LUAI_FUNC void luaX_setinput (lua_State *L, LexState *ls, ZIO *z,
                              TString *source, int firstchar);
LUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC void luaX_next (LexState *ls);
LUAI_FUNC int luaX_lookahead (LexState *ls);
LUAI_FUNC l_noret luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);
LUAI_FUNC const char *luaX_token2str_noq (LexState *ls, int token);


#endif
