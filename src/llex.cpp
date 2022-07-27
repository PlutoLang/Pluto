/*
** $Id: llex.c $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#define llex_c
#define LUA_CORE

#include "lprefix.h"


#include <locale.h>
#include <string.h>

#include "lua.h"

#include "lctype.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "llex.h"
#include "lobject.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "lzio.h"



#define next(ls)	(ls->current = zgetc(ls->z))


#define check_condition(ls,c,msg)	{ if (!(c)) luaX_syntaxerror(ls, msg); }


#define currIsNewline(ls)	(ls->current == '\n' || ls->current == '\r')


/* ORDER RESERVED */
static const char *const luaX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "pluto_switch", "pluto_case", "pluto_default", "pluto_continue", "pluto_when",
#ifndef PLUTO_COMPATIBLE_SWITCH
    "switch",
#endif
#ifndef PLUTO_COMPATIBLE_CASE
    "case",
#endif
#ifndef PLUTO_COMPATIBLE_DEFAULT
    "default",
#endif
#ifndef PLUTO_COMPATIBLE_CONTINUE
    "continue",
#endif
#ifndef PLUTO_COMPATIBLE_WHEN
    "when",
#endif
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};


#define save_and_next(ls) (save(ls, ls->current), next(ls))


[[noreturn]] static void lexerror (LexState *ls, const char *msg, int token);


static void save (LexState *ls, int c) {
  Mbuffer *b = ls->buff;
  if (luaZ_bufflen(b) + 1 > luaZ_sizebuffer(b)) {
    size_t newsize;
    if (luaZ_sizebuffer(b) >= MAX_SIZE/2)
      lexerror(ls, "lexical element too long", 0);
    newsize = luaZ_sizebuffer(b) * 2;
    luaZ_resizebuffer(ls->L, b, newsize);
  }
  b->buffer[luaZ_bufflen(b)++] = cast_char(c);
}


void luaX_init (lua_State *L) {
  int i;
  TString *e = luaS_newliteral(L, LUA_ENV);  /* create env name */
  luaC_fix(L, obj2gco(e));  /* never collect this name */
  for (i=0; i<NUM_RESERVED; i++) {
    TString *ts = luaS_new(L, luaX_tokens[i]);
    luaC_fix(L, obj2gco(ts));  /* reserved words are never collected */
    ts->extra = cast_byte(i+1);  /* reserved word */
  }
}


const char *luaX_token2str (LexState *ls, int token) {
  if (token < FIRST_RESERVED) {  /* single-byte symbols? */
    if (lisprint(token))
      return luaO_pushfstring(ls->L, "'%c'", token);
    else  /* control character */
      return luaO_pushfstring(ls->L, "'<\\%d>'", token);
  }
  else {
    const char *s = luaX_tokens[token - FIRST_RESERVED];
    if (token < TK_EOS)  /* fixed format (symbols and reserved words)? */
      return luaO_pushfstring(ls->L, "'%s'", s);
    else  /* names, strings, and numerals */
      return s;
  }
}


/* Converts a token into a string, same as luaX_token2str (but it doesn't quote the token). */
const char *luaX_token2str_noq (LexState *ls, int token) {
  if (token < FIRST_RESERVED) {  /* single-byte symbols? */
    if (lisprint(token))
      return luaO_pushfstring(ls->L, "%c", token);
    else  /* control character */
      return luaO_pushfstring(ls->L, "'<\\%d>'", token);
  }
  else {
    const char *s = luaX_tokens[token - FIRST_RESERVED];
    if (token < TK_EOS)  /* fixed format (symbols and reserved words)? */
      return luaO_pushfstring(ls->L, "%s", s);
    else  /* names, strings, and numerals */
      return s;
  }
}


const char* luaX_reserved2str (int token) {
  return luaX_tokens[token - FIRST_RESERVED];
}


static const char *txtToken (LexState *ls, int token) {
  switch (token) {
    case TK_NAME: case TK_STRING:
    case TK_FLT: case TK_INT:
      save(ls, '\0');
      return luaO_pushfstring(ls->L, "'%s'", luaZ_buffer(ls->buff));
    default:
      return luaX_token2str(ls, token);
  }
}


[[noreturn]] static void lexerror (LexState *ls, const char *msg, int token) {
  msg = luaG_addinfo(ls->L, msg, ls->source, ls->getLineNumber());
  if (token)
    luaO_pushfstring(ls->L, "%s near %s", msg, txtToken(ls, token));
  luaD_throw(ls->L, LUA_ERRSYNTAX);
}


void luaX_syntaxerror (LexState *ls, const char *msg) {
  lexerror(ls, msg, ls->t.token);
}


/*
** Creates a new string and anchors it in scanner's table so that it
** will not be collected until the end of the compilation; by that time
** it should be anchored somewhere. It also internalizes long strings,
** ensuring there is only one copy of each unique string.  The table
** here is used as a set: the string enters as the key, while its value
** is irrelevant. We use the string itself as the value only because it
** is a TValue readly available. Later, the code generation can change
** this value.
*/
TString *luaX_newstring (LexState *ls, const char *str, size_t l) {
  lua_State *L = ls->L;
  TString *ts = luaS_newlstr(L, str, l);  /* create new string */
  const TValue *o = luaH_getstr(ls->h, ts);
  if (!ttisnil(o))  /* string already present? */
    ts = keystrval(nodefromval(o));  /* get saved copy */
  else {  /* not in use yet */
    TValue *stv = s2v(L->top++);  /* reserve stack space for string */
    setsvalue(L, stv, ts);  /* temporarily anchor the string */
    luaH_finishset(L, ls->h, stv, o, stv);  /* t[string] = string */
    /* table is not a metatable, so it does not need to invalidate cache */
    luaC_checkGC(L);
    L->top--;  /* remove string from stack */
  }
  return ts;
}


LUAI_FUNC TString* luaX_newstring (LexState *ls, const char *str) {
  return luaX_newstring(ls, str, strlen(str));
}


/*
** increment line number and skips newline sequence (any of
** \n, \r, \n\r, or \r\n)
*/
static void inclinenumber (LexState *ls) {
  int old = ls->current;
  lua_assert(currIsNewline(ls));
  next(ls);  /* skip '\n' or '\r' */
  if (currIsNewline(ls) && ls->current != old)
    next(ls);  /* skip '\n\r' or '\r\n' */
  ls->lines.emplace_back(std::string{});
}


void luaX_setinput (lua_State *L, LexState *ls, ZIO *z, TString *source,
                    int firstchar) {
  ls->t.token = 0;
  ls->lasttoken = 0;
  ls->L = L;
  ls->current = firstchar;
  ls->lookahead.token = TK_EOS;  /* no look-ahead token */
  ls->z = z;
  ls->fs = NULL;
  ls->source = source;
  ls->envn = luaS_newliteral(L, LUA_ENV);  /* get env name */
  luaZ_resizebuffer(ls->L, ls->buff, LUA_MINBUFFER);  /* initialize buffer */
}



/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/


static int check_next1 (LexState *ls, int c) {
  if (ls->current == c) {
    next(ls);
    return 1;
  }
  else return 0;
}


/*
** Check whether current char is in set 'set' (with two chars) and
** saves it
*/
static int check_next2 (LexState *ls, const char *set) {
  lua_assert(set[2] == '\0');
  if (ls->current == set[0] || ls->current == set[1]) {
    save_and_next(ls);
    return 1;
  }
  else return 0;
}


/* LUA_NUMBER */
/*
** This function is quite liberal in what it accepts, as 'luaO_str2num'
** will reject ill-formed numerals. Roughly, it accepts the following
** pattern:
**
**   %d(%x|%.|([Ee][+-]?))* | 0[Xx](%x|%.|([Pp][+-]?))*
**
** The only tricky part is to accept [+-] only after a valid exponent
** mark, to avoid reading '3-4' or '0xe+1' as a single number.
**
** The caller might have already read an initial dot.
*/
static int read_numeral (LexState *ls, SemInfo *seminfo) {
  TValue obj;
  const char *expo = "Ee";
  int first = ls->current;
  lua_assert(lisdigit(ls->current));
  save_and_next(ls);
  if (first == '0' && check_next2(ls, "xX"))  /* hexadecimal? */
    expo = "Pp";
  for (;;) {
    if (check_next2(ls, expo))  /* exponent mark? */
      check_next2(ls, "-+");  /* optional exponent sign */
    else if (lisxdigit(ls->current) || ls->current == '.' || ls->current == '_') { /* '%x|%.' */
      if (ls->current != '_')
        save_and_next(ls);
      else
        next(ls);
    }
    else break;
  }
  if (lislalpha(ls->current))  /* is numeral touching a letter? */
    save_and_next(ls);  /* force an error */
  save(ls, '\0');
  if (luaO_str2num(luaZ_buffer(ls->buff), &obj) == 0)  /* format error? */
    lexerror(ls, "malformed number", TK_FLT);
  if (ttisinteger(&obj)) {
    seminfo->i = ivalue(&obj);
    return TK_INT;
  }
  else {
    lua_assert(ttisfloat(&obj));
    seminfo->r = fltvalue(&obj);
    return TK_FLT;
  }
}


/*
** read a sequence '[=*[' or ']=*]', leaving the last bracket. If
** sequence is well formed, return its number of '='s + 2; otherwise,
** return 1 if it is a single bracket (no '='s and no 2nd bracket);
** otherwise (an unfinished '[==...') return 0.
*/
static size_t skip_sep (LexState *ls) {
  size_t count = 0;
  int s = ls->current;
  lua_assert(s == '[' || s == ']');
  save_and_next(ls);
  while (ls->current == '=') {
    save_and_next(ls);
    count++;
  }
  return (ls->current == s) ? count + 2
         : (count == 0) ? 1
         : 0;
}


static void read_long_string (LexState *ls, SemInfo *seminfo, size_t sep) {
  int line = ls->getLineNumber();  /* initial line (for error message) */
  save_and_next(ls);  /* skip 2nd '[' */
  if (currIsNewline(ls))  /* string starts with a newline? */
    inclinenumber(ls);  /* skip it */
  for (;;) {
    switch (ls->current) {
      case EOZ: {  /* error */
        const char *what = (seminfo ? "string" : "comment");
        const char *msg = luaO_pushfstring(ls->L,
                     "unfinished long %s (starting at line %d)", what, line);
        lexerror(ls, msg, TK_EOS);
        break;  /* to avoid warnings */
      }
      case ']': {
        if (skip_sep(ls) == sep) {
          save_and_next(ls);  /* skip 2nd ']' */
          goto endloop;
        }
        break;
      }
      case '\n': case '\r': {
        save(ls, '\n');
        inclinenumber(ls);
        if (!seminfo) luaZ_resetbuffer(ls->buff);  /* avoid wasting space */
        break;
      }
      default: {
        if (seminfo) save_and_next(ls);
        else next(ls);
      }
    }
  } endloop:
  if (seminfo)
    seminfo->ts = luaX_newstring(ls, luaZ_buffer(ls->buff) + sep,
                                     luaZ_bufflen(ls->buff) - 2 * sep);
}


static void esccheck (LexState *ls, int c, const char *msg) {
  if (!c) {
    if (ls->current != EOZ)
      save_and_next(ls);  /* add current to buffer for error message */
    lexerror(ls, msg, TK_STRING);
  }
}


static int gethexa (LexState *ls) {
  save_and_next(ls);
  esccheck (ls, lisxdigit(ls->current), "hexadecimal digit expected");
  return luaO_hexavalue(ls->current);
}


static int readhexaesc (LexState *ls) {
  int r = gethexa(ls);
  r = (r << 4) + gethexa(ls);
  luaZ_buffremove(ls->buff, 2);  /* remove saved chars from buffer */
  return r;
}


static unsigned long readutf8esc (LexState *ls) {
  unsigned long r;
  int i = 4;  /* chars to be removed: '\', 'u', '{', and first digit */
  save_and_next(ls);  /* skip 'u' */
  esccheck(ls, ls->current == '{', "missing '{'");
  r = gethexa(ls);  /* must have at least one digit */
  while (cast_void(save_and_next(ls)), lisxdigit(ls->current)) {
    i++;
    esccheck(ls, r <= (0x7FFFFFFFu >> 4), "UTF-8 value too large");
    r = (r << 4) + luaO_hexavalue(ls->current);
  }
  esccheck(ls, ls->current == '}', "missing '}'");
  next(ls);  /* skip '}' */
  luaZ_buffremove(ls->buff, i);  /* remove saved chars from buffer */
  return r;
}


static void utf8esc (LexState *ls) {
  char buff[UTF8BUFFSZ];
  int n = luaO_utf8esc(buff, readutf8esc(ls));
  for (; n > 0; n--)  /* add 'buff' to string */
    save(ls, buff[UTF8BUFFSZ - n]);
}


static int readdecesc (LexState *ls) {
  int i;
  int r = 0;  /* result accumulator */
  for (i = 0; i < 3 && lisdigit(ls->current); i++) {  /* read up to 3 digits */
    r = 10*r + ls->current - '0';
    save_and_next(ls);
  }
  esccheck(ls, r <= UCHAR_MAX, "decimal escape too large");
  luaZ_buffremove(ls->buff, i);  /* remove read digits from buffer */
  return r;
}


static void read_string (LexState *ls, int del, SemInfo *seminfo) {
  next(ls);  /* keep delimiter (for error messages) */
  while (ls->current != del) {
    switch (ls->current) {
      case EOZ:
        lexerror(ls, "unfinished string", TK_EOS);
        break;  /* to avoid warnings */
      case '\n':
      case '\r':
        lexerror(ls, "unfinished string", TK_STRING);
        break;  /* to avoid warnings */
      case '\\': {  /* escape sequences */
        int c;  /* final character to be saved */
        save_and_next(ls);  /* keep '\\' for error messages */
        switch (ls->current) {
          case 'a': c = '\a'; goto read_save;
          case 'b': c = '\b'; goto read_save;
          case 'f': c = '\f'; goto read_save;
          case 'n': c = '\n'; goto read_save;
          case 'r': c = '\r'; goto read_save;
          case 't': c = '\t'; goto read_save;
          case 'v': c = '\v'; goto read_save;
          case 'x': c = readhexaesc(ls); goto read_save;
          case 'u': utf8esc(ls);  goto no_save;
          case '\n': case '\r':
            inclinenumber(ls); c = '\n'; goto only_save;
          case '\\': case '\"': case '\'':
            c = ls->current; goto read_save;
          case EOZ: goto no_save;  /* will raise an error next loop */
          case 'z': {  /* zap following span of spaces */
            luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
            next(ls);  /* skip the 'z' */
            while (lisspace(ls->current)) {
              if (currIsNewline(ls)) inclinenumber(ls);
              else next(ls);
            }
            goto no_save;
          }
          default: {
            esccheck(ls, lisdigit(ls->current), "invalid escape sequence");
            c = readdecesc(ls);  /* digital escape '\ddd' */
            goto only_save;
          }
        }
       read_save:
         next(ls);
         /* go through */
       only_save:
         luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
         save(ls, c);
         /* go through */
       no_save: break;
      }
      default:
        save_and_next(ls);
    }
  }
  next(ls);  /* skip delimiter */
  seminfo->ts = luaX_newstring(ls, luaZ_buffer(ls->buff), luaZ_bufflen(ls->buff));
  if (ls->current == del) {  // Chained, implicit string literal concatenation.
    read_string(ls, del, seminfo);
  }
}

/* assigns a reserved augmentation symbol to the lexer state token (ls->lasttoken) */
static int llex_augmented (LexState *ls, int c) {
  switch (c) {
    case '+': {
      ls->lasttoken = TK_CADD;
      return 1;
    }
    case '*': {
      ls->lasttoken = TK_CMUL;
      return 1;
    }
    case '%': {
      ls->lasttoken = TK_CMOD;
      return 1;
    }
    case '^': {
      ls->lasttoken = TK_CPOW;
      return 1;
    }
    case '|': {
      ls->lasttoken = TK_CBOR;
      return 1;
    }
    case '&': {
      ls->lasttoken = TK_CBAND;
      return 1;
    }
    default: {
      return 0;  /* failure */
    }
  }
}

static int llex (LexState *ls, SemInfo *seminfo) {
  luaZ_resetbuffer(ls->buff);
  for (;;) {
    switch (ls->current) {
      case '\n': case '\r': {  /* Line breaks. */
        inclinenumber(ls);
        break;
      }
      case ' ': case '\f': case '\t': case '\v': {  /* spaces */
        if (ls->getLineBuff().find_first_not_of(ls->current) != std::string::npos) {
          ls->appendLineBuff(ls->current);
        }
        next(ls);
        break;
      }
      case '-': {  /* '-' or '--' (comment) */
        next(ls);
        if (check_next1(ls, '=')) { /* compound op */
          ls->appendLineBuff("-=");
          ls->lasttoken = TK_CSUB;
          return '=';
        }
        else {
          if (ls->current != '-') {
            ls->appendLineBuff('-');
            return '-';
          }
          /* else is a comment */
          next(ls);
          if (ls->current == '[') {  /* long comment? */
            size_t sep = skip_sep(ls);
            luaZ_resetbuffer(ls->buff);  /* 'skip_sep' may dirty the buffer */
            if (sep >= 2) {
              read_long_string(ls, NULL, sep);  /* skip long comment */
              luaZ_resetbuffer(ls->buff);  /* previous call may dirty the buff. */
              break;
            }
          }
          /* else short comment */
          while (!currIsNewline(ls) && ls->current != EOZ)
            next(ls);  /* skip until end of line (or end of file) */
          break;
        }
      }
      case '[': {  /* long string or simply '[' */
        size_t sep = skip_sep(ls);
        if (sep >= 2) {
          read_long_string(ls, seminfo, sep);
          return TK_STRING;
        }
        else if (sep == 0)  /* '[=...' missing second bracket? */
          lexerror(ls, "invalid long string delimiter", TK_STRING);
        return '[';
      }
      case '=': {
        next(ls);
        if (check_next1(ls, '=')) {
          ls->appendLineBuff("==");
          return TK_EQ;  /* '==' */
        }
        else {
          ls->appendLineBuff('=');
          return '=';
        }
      }
      case '<': {
        next(ls);
        if (check_next1(ls, '=')) {
          ls->appendLineBuff("<=");
          return TK_LE;  /* '<=' */
        }
        else if (check_next1(ls, '<')) {
          if (check_next1(ls, '=')) {  /* compound support */
            ls->appendLineBuff("<<=");
            ls->lasttoken = TK_CSHL;  /* <<= */
            return '=';
          }
          else {
            ls->appendLineBuff("<<");
            return TK_SHL;  /* '<<' */
          }
        }
        else {
          ls->appendLineBuff('<');
          return '<';
        }
      }
      case '>': {
        next(ls);
        if (check_next1(ls, '=')) {
          ls->appendLineBuff(">=");
          return TK_GE;  /* '>=' */
        }
        else if (check_next1(ls, '>')) {
          if (check_next1(ls, '=')) {  /* compound support */
            ls->appendLineBuff(">>=");
            ls->lasttoken = TK_CSHR;  /* >>= */
            return '=';
          }
          else {
            ls->appendLineBuff(">>");
            return TK_SHR;  /* '>>' */
          }
        }
        else {
          ls->appendLineBuff('>');
          return '>';
        }
      }
      case '/': {
        next(ls);
        if (check_next1(ls, '=')) {  /* compound support */
          ls->appendLineBuff("/=");
          ls->lasttoken = TK_CDIV;
          return '=';
        } else {
          if (check_next1(ls, '/')) {
            if (!check_next1(ls, '=')) {
              ls->appendLineBuff("//");
              return TK_IDIV;  /* '//' */
            }
            else {  /* floor division compound support */
              ls->appendLineBuff("//=");
              ls->lasttoken = TK_CIDIV;
              return '=';
            }
          }
          else {
            ls->appendLineBuff('/');
            return '/';
          }
        }
      }
      case ':': {
        next(ls);
        if (check_next1(ls, ':')) {
          ls->appendLineBuff("::");
          return TK_DBCOLON;  /* '::' */
        }
        else {
          ls->appendLineBuff(':');
          return ':';
        }
      }
      case '"': case '\'': {  /* short literal strings */
        read_string(ls, ls->current, seminfo);
        ls->appendLineBuff("\"");
        ls->appendLineBuff(seminfo->ts->contents);
        ls->appendLineBuff("\"");
        return TK_STRING;
      }
      case '.': {  /* '.', '..', '...', or number */
        save_and_next(ls);
        if (check_next1(ls, '.')) {
          if (check_next1(ls, '.')) {
            ls->appendLineBuff("...");
            return TK_DOTS;   /* '...' */
          }
          else {
            if (check_next1(ls, '=')) {
              ls->appendLineBuff("..=");
              ls->lasttoken = TK_CCAT;
              return '=';
            } else {
              ls->appendLineBuff("..");
              return TK_CONCAT;   /* '..' */
            }
          }
        }
        else if (!lisdigit(ls->current)) {
          ls->appendLineBuff('.');
          return '.';
        }
        else {
          int ret = read_numeral(ls, seminfo);
          if (ret == TK_INT)
            ls->appendLineBuff(std::to_string(seminfo->i));
          else
            ls->appendLineBuff(std::to_string(seminfo->r));
          return ret;
        }
      }
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        int ret = read_numeral(ls, seminfo);
        if (ret == TK_INT)
          ls->appendLineBuff(std::to_string(seminfo->i));
        else
          ls->appendLineBuff(std::to_string(seminfo->r));
        return ret;
      }
      case '_': {  /* arbitrary underscores for more readable long numbers */
        if (lislalpha(ls->current)) {
          TString *ts;
          do {
            save_and_next(ls);
          } while (lislalnum(ls->current));
          ts = luaX_newstring(ls, luaZ_buffer(ls->buff),
                                  luaZ_bufflen(ls->buff));
          seminfo->ts = ts;
          ls->appendLineBuff(ts->contents);
          if (isreserved(ts))
            return ts->extra - 1 + FIRST_RESERVED;
          else {
            return TK_NAME;
          }
        } else {  /* needed to emulate default check because _ is fairly multi-purpose. */
          next(ls);
          if (lisdigit(ls->current)) {
            int ret = read_numeral(ls, seminfo);
            if (ret == TK_INT)
              ls->appendLineBuff(std::to_string(seminfo->i));
            else
              ls->appendLineBuff(std::to_string(seminfo->r));
            return ret;  /* arbitrary character detected in numeral */
          } else {
            ls->appendLineBuff('_');
            return '_';  /* this is a normal underscore */
          }
        }
      }
      case '~': {
        next(ls);
        if (check_next1(ls, '=')) {
          ls->appendLineBuff("~=");
          return TK_NE;
        } else {
          ls->appendLineBuff('~');
          return '~';
        }
      }
      case '!': {
        next(ls);
        if (check_next1(ls, '=')) return TK_NE;  /* '!=' */
        else return '!';
      }
      case '?': {
        next(ls);
        if (check_next1(ls, '?')) {
          if (check_next1(ls, '=')) {
            ls->lasttoken = TK_COAL;
            return '=';
          } else return TK_COAL; /* ?? */
        }
        else return '?';
      }
      case '*': {  /* special case compound, need to support mul, exponent, and augmented mul */
        next(ls);
        if (check_next1(ls, '=')) {
          ls->appendLineBuff("*=");
          ls->lasttoken = TK_CMUL;
          return '=';  /* '*=' */
        }
        else if (check_next1(ls, '*')) { /*  got '**' */  
          if (check_next1(ls, '=')) {  /* compound support; **= */
            ls->appendLineBuff("**=");
            ls->lasttoken = TK_CPOW;
            return '=';
          }
          else {
            ls->appendLineBuff("**");
            return TK_POW;  /* '**' */
          }
        }
        else {
          ls->appendLineBuff('*');
          return '*';
        }
      }
      /* compound support */
      case '+':
      case '^': case '%':
      case '|': case '&': { 
        int c = ls->current;
        next(ls);
        if (check_next1(ls, '=')) {
          if (llex_augmented(ls, c) != 1) {
            lexerror(ls, "unsupported augmented assignment", c);
          } else {
            ls->appendLineBuff(c);
            ls->appendLineBuff('=');
            return '=';
          }
        } else {
          ls->appendLineBuff(c);
          return c;
        }
      }
      /* end of compound support */
      case EOZ: {
        return TK_EOS;
      }
      default: {
        if (lislalpha(ls->current)) {  /* identifier or reserved word? */
          TString *ts;
          do {
            save_and_next(ls);
          } while (lislalnum(ls->current));
          ts = luaX_newstring(ls, luaZ_buffer(ls->buff),
                                  luaZ_bufflen(ls->buff));
          seminfo->ts = ts;
          ls->appendLineBuff(ts->contents);
          if (isreserved(ts))  /* reserved word? */
            return ts->extra - 1 + FIRST_RESERVED;
          else {
            return TK_NAME;
          }
        }
        else {  /* single-char tokens ('+', '*', '%', '{', '}', ...) */
          int c = ls->current;
          next(ls);
          ls->appendLineBuff(c);
          return c;
        }
      }
    }
  }
}


void luaX_next (LexState *ls) {
  ls->lastline = ls->getLineNumber();
  if (ls->lookahead.token != TK_EOS) {  /* is there a look-ahead token? */
    ls->t = ls->lookahead;  /* use this one */
    ls->lookahead.token = TK_EOS;  /* and discharge it */
  }
  else
    ls->t.token = llex(ls, &ls->t.seminfo);  /* read next token */
}


int luaX_lookahead (LexState *ls) {
  lua_assert(ls->lookahead.token == TK_EOS);
  ls->lookahead.token = llex(ls, &ls->lookahead.seminfo);
  return ls->lookahead.token;
}
