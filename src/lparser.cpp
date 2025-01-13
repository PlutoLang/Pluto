/*
** $Id: lparser.c $
** Lua Parser
** See Copyright Notice in lua.h
*/

#define lparser_c
#define LUA_CORE

#include "lprefix.h"


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include <functional>
#include <string>
#include <vector>

#include "lua.h"
#include "lualib.h" // Pluto::all_preloaded
#include "lapi.h" // api_incr_top
#include "lcode.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "lsuggestions.hpp"
#include "ltable.h"

#include "lerrormessage.hpp"

#include "vendor/Soup/soup/version_compare.hpp"



#define hasmultret(k)		((k) == VCALL || (k) == VVARARG)


/*
** Invokes the lua_writestring macro with a std::string.
*/
#define write_std_string(std_string) lua_writestring(std_string.data(), std_string.size())


/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define eqstr(a,b)	((a) == (b))


#define luaO_fmt luaO_pushfstring


//#define PLUTO_ALLOW_FUNCTION_INJECTION_REASSIGNMENT


std::string TypeDesc::toString() const {
  std::string str = vtToString(type);
  if (type == VT_FUNC &&
      retn &&
      !retn->empty()) {
    str.push_back('(');
    str.append(retn->toString());
    str.push_back(')');
  }
  return str;
}

enum class BlockType : lu_byte {
  BT_DEFAULT,
  BT_BREAK,
  BT_CONTINUE
};

/*
** nodes for block list (list of active blocks)
*/
typedef struct BlockCnt {
  struct BlockCnt *previous;  /* chain */
  int firstlabel;  /* index of first label in this block */
  int firstgoto;  /* index of first pending goto in this block */
  int nactvar;  /* # active locals outside the block */
  lu_byte upval;  /* true if some variable in the block is an upvalue */
  BlockType type;  /* one of block types */
  lu_byte insidetbc;  /* true if inside the scope of a to-be-closed var. */
  std::vector<TString*> export_symbols{};
  ValType var_overide = VT_NONE;
  unsigned short var_overide_vidx;
} BlockCnt;



/*
** prototypes for recursive non-terminal functions
*/
static void statement (LexState *ls, TypeHint *prop = nullptr);
static void expr (LexState *ls, expdesc *v, TypeHint *prop = nullptr, int flags = 0);


/*
** Throws an exception into Lua, which will promptly close the program.
** This is only called for vital errors, like lexer and/or syntax problems.
*/
static l_noret throwerr (LexState *ls, const char *err, const char *here, int line, const char *note = nullptr) {
  err = luaG_addinfo(ls->L, err, ls->source, line);
  auto msg = new Pluto::ErrorMessage{ ls, HRED "syntax error: " BWHT }; // We'll only throw syntax errors if 'throwerr' is called
  msg->addMsg(err);
  if (ls->t.token == TK_EOS && strstr(err, "near '<eof>'") == nullptr) {  /* for 'incomplete' in REPL */
    msg->addMsg(" near ")
       .addMsg(luaX_token2str(ls, ls->t));
  }
  msg->addSrcLine(line)
     .addGenericHere(here);

  if (note != nullptr) {
    msg->addNote(note);
  }

  msg->finalizeAndThrow();
}

static l_noret throwerr (LexState *ls, const char *err, const char *here, const char *note = nullptr) {
  throwerr(ls, err, here, ls->getLineNumber(), note);
}


// No note.
static void throw_warn (LexState *ls, const char *err, const char *here, int line, WarningType warningType) {
  if (ls->shouldEmitWarning(line, warningType)) {
    auto msg = new Pluto::ErrorMessage{ ls, luaG_addinfo(ls->L, YEL "warning: " BWHT, ls->source, line) };
    msg->addMsg(err)
      .addMsg(" [")
      .addMsg(ls->getWarningConfig().getWarningName(warningType))
      .addMsg("]")
      .addSrcLine(line)
      .addGenericHere(here)
      .finalize();
    if (ls->getWarningConfig().isFatal(warningType)) {
      delete msg;
      luaD_throw(ls->L, LUA_ERRSYNTAX);
    }
    lua_warning(ls->L, msg->content.c_str(), 0);
    delete msg;
    ls->L->top.p -= 2; // Pluto::ErrorMessage::finalize & luaG_addinfo
  }
}

// Note.
static void throw_warn(LexState* ls, const char* err, const char* here, const char* note, int line, WarningType warningType) {
  if (ls->shouldEmitWarning(line, warningType)) {
    auto msg = new Pluto::ErrorMessage{ ls, luaG_addinfo(ls->L, YEL "warning: " BWHT, ls->source, line) };
    msg->addMsg(err)
      .addMsg(" [")
      .addMsg(ls->getWarningConfig().getWarningName(warningType))
      .addMsg("]")
      .addSrcLine(line)
      .addGenericHere(here)
      .addNote(note)
      .finalize();
    if (ls->getWarningConfig().isFatal(warningType)) {
      delete msg;
      luaD_throw(ls->L, LUA_ERRSYNTAX);
    }
    lua_warning(ls->L, msg->content.c_str(), 0);
    delete msg;
    ls->L->top.p -= 2; // Pluto::ErrorMessage::finalize & luaG_addinfo
  }
}

static void throw_warn(LexState *ls, const char* err, const char *here, WarningType warningType) {
  return throw_warn(ls, err, here, ls->getLineNumber(), warningType);
}

static void throw_warn(LexState *ls, const char *err, int line, WarningType warningType) {
  return throw_warn(ls, err, "", line, warningType);
}

#pragma warning(disable : 4068) // unknown pragma
#pragma clang diagnostic ignored "-Wunused-function"
static void throw_warn(LexState* ls, const char* err, WarningType warningType) {
  throw_warn(ls, err, ls->getLineNumber(), warningType);
}


static void disablekeyword (LexState *ls, int token) {
  auto i = ls->tokens.begin();
  if (ls->tidx != -1)
    i += ls->tidx;  /* don't apply retroactively */
  for (; i != ls->tokens.end(); ++i)
    if (i->token == token)
      i->token = TK_NAME;
}

/*
** Responsible for the following:
**   - Non-portable keyword usage. (class, switch, etc)
*/
static void check_for_non_portable_code (LexState *ls) {
  if (ls->t.IsNonCompatible() && !ls->t.IsOverridable()) {
    if (ls->getKeywordState(ls->t.token) == KS_ENABLED_BY_PLUTO_UNINFORMED) {
      const auto next = luaX_lookahead(ls);
      if (next == '=' || next == '.' || next == ':' || next == '['  /* attempting to create or use a global? */
#ifdef PLUTO_PARANOID_KEYWORD_DETECTION
          || next == '(' || next == '{' || next == TK_STRING
#endif
        ) {
        disablekeyword(ls, ls->t.token);
        ls->uninformed_reserved.emplace(ls->t.token, ls->getLineNumber());
        ls->setKeywordState(ls->t.token, KS_DISABLED_BY_PLUTO_INFORMED);
        luaX_setpos(ls, luaX_getpos(ls));  /* update ls->t */
      }
      else
        ls->setKeywordState(ls->t.token, KS_ENABLED_BY_PLUTO_INFORMED);
    }
    if (ls->getKeywordState(ls->t.token) == KS_ENABLED_BY_PLUTO_INFORMED || ls->getKeywordState(ls->t.token) == KS_ENABLED_BY_ENV) {  /* enabled by a means other than 'pluto_use'? */
      throw_warn(ls, "non-portable keyword usage", luaO_fmt(ls->L, "use 'pluto_%s' instead, or 'pluto_use' this keyword: https://pluto.do/compat", luaX_token2str_noq(ls, ls->t)), WT_NON_PORTABLE_CODE);
      ls->L->top.p--;
    }
  }
}


/*
** This function will throw an exception and terminate the program.
*/
static l_noret error_expected (LexState *ls, int token) {
  switch (token) {
    case TK_ARROW: {
      if (luaX_lookahead(ls) == '>') {
        throwerr(ls,
          "impromper lambda definition",
          "expected '->' arrow syntax for lambda expression.");
      }
      goto _default; // Run-through default case, no more work to be done.
    }
    case TK_IN: {
      throwerr(ls,
        "expected 'in' to delimit loop iterator.", "expected 'in' symbol.");
    }
    case TK_DO: {
      throwerr(ls,
        "expected 'do' to establish block.", "you need to append this with the 'do' symbol.");
    }
    case TK_END: {
      throwerr(ls,
        "expected 'end' to terminate block.", "expected 'end' symbol after or on this line.");
    }
    case TK_NAME: {
      throwerr(ls,
        "expected an identifier.", "this needs a name.");
    }
    case TK_PCONTINUE: {
      throwerr(ls,
        "expected 'continue' inside a loop.", "this is not within a loop.");
    }
    default: {
      _default:
      throwerr(ls, luaO_fmt(ls->L, "%s expected near %s", luaX_token2str(ls, token), luaX_token2str(ls, ls->t)), "this is invalid syntax.");
    }
  }
}


static l_noret errorlimit (FuncState *fs, int limit, const char *what) {
  lua_State *L = fs->ls->L;
  const char *msg;
  int line = fs->f->linedefined;
  const char *where = (line == 0)
                      ? "main function"
                      : luaO_pushfstring(L, "function at line %d", line);
  msg = luaO_pushfstring(L, "too many %s (limit is %d) in %s",
                             what, limit, where);
  luaX_syntaxerror(fs->ls, msg);
}


static void checklimit (FuncState *fs, int v, int l, const char *what) {
  if (v > l) errorlimit(fs, l, what);
}


/*
** Test whether next token is 'c'; if so, skip it.
*/
static int testnext (LexState *ls, int c) {
  if (ls->t.token == c) {
    luaX_next(ls);
    return 1;
  }
  else return 0;
}


// Test the next token to see if it's either 'token1' or 'token2'.
static bool testnext2 (LexState *ls, int token1, int token2) {
  return testnext(ls, token1) || testnext(ls, token2);
}


/*
** Check that next token is 'c'.
*/
static void check (LexState *ls, int c) {
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", luaX_token2str_noq(ls, c));
  }
#endif
  if (ls->t.token != c) {
    error_expected(ls, c);
  }
}


/*
** Check that next token is 'c' and skip it.
*/
static void checknext (LexState *ls, int c) {
  check(ls, c);
  luaX_next(ls);
}


#define check_condition(ls,c,msg)	{ if (!(c)) luaX_syntaxerror(ls, msg); }


/*
** Check that next token is 'what' and skip it. In case of error,
** raise an error that the expected 'what' should match a 'who'
** in line 'where' (if that is not the current line).
*/
static void check_match (LexState *ls, int what, int who, int where, const char* note = nullptr) {
  if (l_unlikely(!testnext(ls, what))) {
    if (where == ls->getLineNumber())  /* all in the same line? */
      error_expected(ls, what);  /* do not need a complex message */
    else {
      if (what == TK_END) {
        if (ls->else_if)
          throw_warn(ls, "'else if' is not the same as 'elseif' in Lua/Pluto", "did you mean 'elseif'?", ls->else_if, WT_POSSIBLE_TYPO);
        const char *msg;
        if (who == TK_ARROW) {
          msg = luaO_fmt(ls->L, "missing 'end' to terminate lambda starting on line %d", where);
        }
        else {
          msg = luaO_fmt(ls->L, "missing 'end' to terminate %s on line %d", luaX_token2str(ls, who), where);
        }
        throwerr(ls, msg, "this was the last statement.", ls->getLineNumberOfLastNonEmptyLine());
      }
      else {
        auto err = new Pluto::ErrorMessage{ ls, RED "syntax error: " BWHT };
        err->addMsg(luaX_token2str(ls, what))
          .addMsg(" expected (to close ")
          .addMsg(luaX_token2str(ls, who))
          .addMsg(" on line ")
          .addMsg(luaO_fmt(ls->L, "%d", where))
          .addMsg(")")
          .addSrcLine(ls->getLineNumberOfLastNonEmptyLine())
          .addGenericHere();

        if (note != nullptr) {
          err->addNote(note);
        }

        err->finalizeAndThrow();
      }
    }
  }
}


enum NameFlags {
  N_RESERVED_NON_VALUE = (1 << 0),
  N_RESERVED = (1 << 1),
  N_OVERRIDABLE = (1 << 2),
};

[[nodiscard]] static bool isnametkn (LexState *ls, int flags = 0) {
  return ls->t.token == TK_NAME || ls->t.IsNarrow()
      || ((flags & N_RESERVED_NON_VALUE) && ls->t.IsReservedNonValue())
      || ((flags & N_RESERVED) && ls->t.IsReserved())
      || ((flags & N_OVERRIDABLE) && ls->t.IsOverridable())
      ;
}

[[nodiscard]] static int find_non_compat_tkn_by_name (LexState *ls, const char *str) {
  for (int i = FIRST_NON_COMPAT; i != END_NON_COMPAT; ++i) {
    if (strcmp(luaX_token2str_noq(ls, i), str) == 0) {
      return i;
    }
  }
  return 0;
}

static bool trydisablekeyword (LexState *ls) {
  if (ls->getKeywordState(ls->t.token) == KS_ENABLED_BY_PLUTO_UNINFORMED) {
    disablekeyword(ls, ls->t.token);
    ls->uninformed_reserved.emplace(ls->t.token, ls->getLineNumber());
    ls->setKeywordState(ls->t.token, KS_DISABLED_BY_PLUTO_INFORMED);
    luaX_setpos(ls, luaX_getpos(ls));  /* update ls->t */
    return true;
  }
  return false;
}

static TString *str_checkname (LexState *ls, int flags = 0) {
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.pushLocals();
  }
#endif
  if (!isnametkn(ls, flags)) {
    if (ls->t.IsNonCompatible()) {
      if (trydisablekeyword(ls))  /* see if we can fix it */
        return str_checkname(ls, flags);  /* then try again */
      throwerr(ls, luaO_fmt(ls->L, "expected a name, found %s", luaX_token2str(ls, ls->t)), luaO_fmt(ls->L, "%s has a different meaning in Pluto, but you can disable this: https://pluto.do/compat", luaX_token2str(ls, ls->t)));
    }
    error_expected(ls, TK_NAME);
  }
  TString *ts = ls->t.seminfo.ts;
  lua_assert(ts != nullptr);
  if (!(flags & N_RESERVED) && !(flags & N_RESERVED_NON_VALUE)) {
    if (auto t = find_non_compat_tkn_by_name(ls, getstr(ts)); t != 0 && t != TK_PARENT) {
      if (ls->getKeywordState(t) != KS_DISABLED_BY_SCRIPTER) {
        throw_warn(
          ls,
          luaO_fmt(ls->L, "'%s' is a non-portable name", getstr(ts)),
          "use a different name, or use 'pluto_use' to disable this keyword: https://pluto.do/compat",
          WT_NON_PORTABLE_NAME
        );
        ls->L->top.p--;
      }
    }
  }
  luaX_next(ls);
  return ts;
}


static void init_exp (expdesc *e, expkind k, int i) {
  e->f = e->t = NO_JUMP;
  e->k = k;
  e->u.info = e->u.pc = e->u.reg = i;
  e->code_primitive = VT_NONE;
}


static void codestring (expdesc *e, TString *s) {
  e->f = e->t = NO_JUMP;
  e->k = VKSTR;
  e->u.strval = s;
  e->code_primitive = VT_STR;
}


static void codename (LexState *ls, expdesc *e, int flags = 0) {
  codestring(e, str_checkname(ls, flags));
}


/*
** Register a new local variable in the active 'Proto' (for debug
** information).
*/
static int registerlocalvar (LexState *ls, FuncState *fs, TString *varname) {
  Proto *f = fs->f;
  int oldsize = f->sizelocvars;
  luaM_growvector(ls->L, f->locvars, fs->ndebugvars, f->sizelocvars,
                  LocVar, SHRT_MAX, "local variables");
  while (oldsize < f->sizelocvars)
    f->locvars[oldsize++].varname = NULL;
  f->locvars[fs->ndebugvars].varname = varname;
  f->locvars[fs->ndebugvars].startpc = fs->pc;
  luaC_objbarrier(ls->L, f, varname);
  return fs->ndebugvars++;
}


#define new_localvarliteral(ls,v) \
    new_localvar(ls,  \
      luaX_newstring(ls, "" v, (sizeof(v)/sizeof(char)) - 1));


[[nodiscard]] static TypeHint* new_typehint (LexState *ls) {
  return ::new (ls->parse_time_allocations.emplace_back(malloc(sizeof(TypeHint)))) TypeHint();
}


[[nodiscard]] static TypeHint gettypehint (LexState *ls, bool funcret = false) {
  /* TYPEHINT -> [':' TYPEDESC { '|' TYPEDESC } ] */
  TypeHint th;
  if (testnext(ls, ':')) {
    if (testnext(ls, '?'))
      th.emplaceTypeDesc(VT_NIL);
    do {
      TString *ts = str_checkname(ls, N_RESERVED);
      const char *tname = getstr(ts);
      if (strcmp(tname, "number") == 0)
        th.emplaceTypeDesc(VT_NUMBER);
      else if (strcmp(tname, "int") == 0)
        th.emplaceTypeDesc(VT_INT);
      else if (strcmp(tname, "float") == 0)
        th.emplaceTypeDesc(VT_FLT);
      else if (strcmp(tname, "table") == 0)
        th.emplaceTypeDesc(VT_TABLE);
      else if (strcmp(tname, "string") == 0)
        th.emplaceTypeDesc(VT_STR);
      else if (strcmp(tname, "boolean") == 0 || strcmp(tname, "bool") == 0)
        th.emplaceTypeDesc(VT_BOOL);
      else if (strcmp(tname, "function") == 0)
        th.emplaceTypeDesc(VT_FUNC);
      else if (strcmp(tname, "void") == 0) {
        if (funcret) {
          if (!th.empty()) { /* already had a hinted type? */
            luaX_prev(ls);
            throw_warn(ls, "'void' must be the only return type if used", "invalid type hint", WT_TYPE_MISMATCH);
            luaX_next(ls);
          }
          th.emplaceTypeDesc(VT_VOID);
          if (testnext(ls, '|'))
            throw_warn(ls, "'void' must be the only return type if used", "invalid type hint", WT_TYPE_MISMATCH);
          break; /* no further type hints allowed */
        }
        throw_warn(ls, "'void' is not a valid type hint in this context", "invalid type hint", WT_TYPE_MISMATCH);
      }
      else if (strcmp(tname, "userdata") != 0) {
        luaX_prev(ls);
        throw_warn(ls, luaO_fmt(ls->L, "'%s' is not a type known to the parser", tname), "unknown type hint", WT_TYPE_MISMATCH);
        ls->L->top.p--;
        luaX_next(ls);
      }
    } while (testnext(ls, '|'));
    if (!th.contains(VT_NIL) && testnext(ls, '?'))
      th.emplaceTypeDesc(VT_NIL);
  }
  return th;
}


/*
** Propgates type from 'e' into 't'.
*/
static void exp_propagate(LexState* ls, const expdesc& e, TypeHint& t) noexcept {
  if (e.k == VLOCAL) {
    if (ls->fs->bl->var_overide != VT_NONE && ls->fs->bl->var_overide_vidx == e.u.var.vidx) {
      t.merge(ls->fs->bl->var_overide);
    }
    else {
      t.merge(*getlocalvardesc(ls->fs, e.u.var.vidx)->vd.prop);
    }
  }
  else if (e.k == VCONST) {
    TValue* val = &ls->dyd->actvar.arr[e.u.info].k;
    switch (ttype(val))
    {
    case LUA_TNIL: t.emplaceTypeDesc(VT_NIL); break;
    case LUA_TBOOLEAN: t.emplaceTypeDesc(VT_BOOL); break;
    case LUA_TNUMBER: t.emplaceTypeDesc((ttypetag(val) == LUA_VNUMINT) ? VT_INT : VT_FLT); break;
    case LUA_TSTRING: t.emplaceTypeDesc(VT_STR); break;
    case LUA_TTABLE: t.emplaceTypeDesc(VT_TABLE); break;
    case LUA_TFUNCTION: t.emplaceTypeDesc(VT_FUNC); break;
    }
  }
}


static void process_assign (LexState *ls, int vidx, const TypeHint& t, int line) {
  Vardesc *var = getlocalvardesc(ls->fs, vidx);
  auto hinted = !var->vd.hint->empty();
  auto knownvalue = !t.empty();
  auto incompatible = !var->vd.hint->isCompatibleWith(t);
  if (hinted && knownvalue && incompatible) {
    const auto hint = var->vd.hint->toString();
    std::string err = var->vd.name->toCpp();
    err.insert(0, "'");
    err.append("' type-hinted as '" + hint);
    err.append("', but provided with ");
    err.append(t.toString());
    err.append(" value.");
    if (t.toPrimitive() == VT_NIL) {  /* Specialize warnings for nullable state incompatibility. */
      throw_warn(ls, "variable type mismatch", err.c_str(), luaO_fmt(ls->L, "try a nilable type hint: '?%s'", hint.c_str()), line, WT_TYPE_MISMATCH);
      ls->L->top.p--; // luaO_fmt
    }
    else {  /* Throw a generic mismatch warning. */
      throw_warn(ls, "variable type mismatch", err.c_str(), line, WT_TYPE_MISMATCH);
    }
  }
  var->vd.prop->merge(t); /* propagate type */
  if (ls->fs->bl->var_overide != VT_NONE && ls->fs->bl->var_overide_vidx == vidx) {
    ls->fs->bl->var_overide = t.toPrimitive();
  }
}


static TypeHint& get_global_prop(LexState* ls, const TString* name) {
  if (auto e = ls->global_props.find(name); e != ls->global_props.end()) {
    return *reinterpret_cast<TypeHint*>(e->second);
  }
  auto th = new_typehint(ls);
  ls->global_props.emplace(name, th);
  return *th;
}


/*
** Convert 'nvar', a compiler index level, to its corresponding
** register. For that, search for the highest variable below that level
** that is in a register and uses its register index ('ridx') plus one.
*/
static int reglevel (FuncState *fs, int nvar) {
  while (nvar-- > 0) {
    Vardesc *vd = getlocalvardesc(fs, nvar);  /* get previous variable */
    if (vd->vd.kind != RDKCTC && vd->vd.kind != RDKENUM)  /* is in a register? */
      return vd->vd.ridx + 1;
  }
  return 0;  /* no variables in registers */
}


/*
** Return the number of variables in the register stack for the given
** function.
*/
int luaY_nvarstack (FuncState *fs) {
  return reglevel(fs, fs->nactvar);
}


/*
** Get the debug-information entry for current variable 'vidx'.
*/
static LocVar *localdebuginfo (FuncState *fs, int vidx) {
  Vardesc *vd = getlocalvardesc(fs, vidx);
  if (vd->vd.kind == RDKCTC || vd->vd.kind == RDKENUM)
    return NULL;  /* no debug info. for constants [Pluto] and named enums */
  else {
    int idx = vd->vd.pidx;
    lua_assert(idx < fs->ndebugvars);
    return &fs->f->locvars[idx];
  }
}


/*
** Arbitrary selection. Based on probability to cause a confusing error (i.e, something that can be more deep than 'attempt to call a string value')
** For example, shadowing 'arg' with a function makes an obvious error. But, shadowing with string or table can cause confusing bugs because they can both be indexed and won't raise an error.
*/
static const char* const common_global_names[] = { PLUTO_COMMON_GLOBAL_NAMES };


static int searchvar (FuncState *fs, TString *n, expdesc *var);
static void checkforshadowing (LexState *ls, FuncState *fs, TString *name, int line, bool check_globals = true, bool check_locals = true) {
  std::string n = name->toCpp();
  if (n == "(for state)" || n == "(switch control value)" || n == "(try results)")
    return;
  FuncState *current_fs = fs;
  while (current_fs != nullptr) {
    if (check_locals) {
      expdesc var;
      if (searchvar(current_fs, name, &var) != -1) {
        Vardesc* desc = getlocalvardesc(current_fs, var.u.var.vidx);
        LocVar* local = localdebuginfo(current_fs, var.u.var.vidx);
        if (local && local->varname == name) { // Got a match.
          throw_warn(ls,
            "duplicate local declaration",
              luaO_fmt(ls->L, "this shadows the initial declaration of '%s' on line %d.", getstr(name), desc->vd.line), line, WT_VAR_SHADOW);
          ls->L->top.p--; /* pop result of luaO_fmt */
          return;
        }
      }
    }
    if (check_globals) {
      for (const auto& global_name : common_global_names) {
        if (n == global_name) {
          throw_warn(ls,
            "duplicate global declaration",
              luaO_fmt(ls->L, "this shadows the initial global definition of '%s'", getstr(name)), line, WT_GLOBAL_SHADOW);
          ls->L->top.p--;
          return;
        }
      }
    }
    current_fs = current_fs->prev;
  }
}


static void checkforshadowing (LexState *ls, FuncState *fs, const std::unordered_set<TString*>& names, int line) {
  for (auto variable_name : names) {
    checkforshadowing(ls, fs, variable_name, line, true, false);
  }
}


/*
** Create a new local variable with the given 'name'. Return its index
** in the function.
*/
static int new_localvar (LexState *ls, TString *name, int line, TypeHint hint = {}, bool check_globals = true) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Dyndata *dyd = ls->dyd;
  Vardesc *var;
  checkforshadowing(ls, fs, name, line, check_globals, true);
  luaM_growvector(L, dyd->actvar.arr, dyd->actvar.n + 1,
                  dyd->actvar.size, Vardesc, SHRT_MAX, "local variables");
  var = &dyd->actvar.arr[dyd->actvar.n++];
  var->vd.kind = VDKREG;  /* default */
  var->vd.hint = new_typehint(ls);
  var->vd.prop = new_typehint(ls);
  if (!hint.empty())
    *var->vd.hint = std::move(hint);
  var->vd.name = name;
  var->vd.line = line;
  return dyd->actvar.n - 1 - fs->firstlocal;
}

static int new_localvar (LexState *ls, TString *name, TypeHint hint = {}) {
  return new_localvar(ls, name, ls->getLineNumber(), std::move(hint));
}


/*
** Create an expression representing variable 'vidx'
*/
static void init_var (FuncState *fs, expdesc *e, int vidx) {
  e->f = e->t = NO_JUMP;
  e->k = VLOCAL;
  e->u.var.vidx = vidx;
  e->u.var.ridx = getlocalvardesc(fs, vidx)->vd.ridx;
}


/*
** Raises an error if variable described by 'e' is read only
*/
static void check_readonly (LexState *ls, expdesc *e) {
  FuncState *fs = ls->fs;
  TString *varname = NULL;  /* to be set if variable is const */
  switch (e->k) {
    case VCONST: {
      varname = ls->dyd->actvar.arr[e->u.info].vd.name;
      break;
    }
    case VLOCAL: {
      Vardesc *vardesc = getlocalvardesc(fs, e->u.var.vidx);
      if (vardesc->vd.kind != VDKREG)  /* not a regular variable? */
        varname = vardesc->vd.name;
      break;
    }
    case VUPVAL: {
      Upvaldesc *up = &fs->f->upvalues[e->u.info];
      if (up->kind != VDKREG)
        varname = up->name;
      break;
    }
    default:
      return;  /* other cases cannot be read-only */
  }
  if (varname) {
    const char *msg = luaO_fmt(ls->L, "attempt to reassign constant '%s'", getstr(varname));
    const char *here = "this variable is constant, and cannot be reassigned.";
    throwerr(ls, luaO_fmt(ls->L, msg, getstr(varname)), here);
  }
}


/*
** Start the scope for the last 'nvars' created variables.
*/
static void adjustlocalvars (LexState *ls, int nvars) {
  FuncState *fs = ls->fs;
  int reglevel = luaY_nvarstack(fs);
  int i;
  for (i = 0; i < nvars; i++) {
    int vidx = fs->nactvar++;
    Vardesc *var = getlocalvardesc(fs, vidx);
    var->vd.ridx = reglevel++;
    var->vd.pidx = registerlocalvar(ls, fs, var->vd.name);
  }
}


/*
** Close the scope for all variables up to level 'tolevel'.
** (debug info.)
*/
static void removevars (FuncState *fs, int tolevel) {
  fs->ls->dyd->actvar.n -= (fs->nactvar - tolevel);
  while (fs->nactvar > tolevel) {
    LocVar *var = localdebuginfo(fs, --fs->nactvar);
    if (var)  /* does it have debug information? */
      var->endpc = fs->pc;
  }
}


/*
** Search the upvalues of the function 'fs' for one
** with the given 'name'.
*/
static int searchupvalue (FuncState *fs, TString *name) {
  int i;
  Upvaldesc *up = fs->f->upvalues;
  for (i = 0; i < fs->nups; i++) {
    if (eqstr(up[i].name, name)) return i;
  }
  return -1;  /* not found */
}


static Upvaldesc *allocupvalue (FuncState *fs) {
  Proto *f = fs->f;
  int oldsize = f->sizeupvalues;
  checklimit(fs, fs->nups + 1, MAXUPVAL, "upvalues");
  luaM_growvector(fs->ls->L, f->upvalues, fs->nups, f->sizeupvalues,
                  Upvaldesc, MAXUPVAL, "upvalues");
  while (oldsize < f->sizeupvalues)
    f->upvalues[oldsize++].name = NULL;
  return &f->upvalues[fs->nups++];
}


static int newupvalue (FuncState *fs, TString *name, expdesc *v) {
  Upvaldesc *up = allocupvalue(fs);
  FuncState *prev = fs->prev;
  if (v->k == VLOCAL) {
    up->instack = 1;
    up->idx = v->u.var.ridx;
    up->kind = getlocalvardesc(prev, v->u.var.vidx)->vd.kind;
    lua_assert(eqstr(name, getlocalvardesc(prev, v->u.var.vidx)->vd.name));
  }
  else {
    lua_assert(v->k == VUPVAL);
    up->instack = 0;
    up->idx = cast_byte(v->u.info);
    up->kind = prev->f->upvalues[v->u.info].kind;
    lua_assert(eqstr(name, prev->f->upvalues[v->u.info].name));
  }
  up->name = name;
  luaC_objbarrier(fs->ls->L, fs->f, name);
  return fs->nups - 1;
}


/*
** Look for an active local variable with the name 'n' in the
** function 'fs'. If found, initialize 'var' with it and return
** its expression kind; otherwise return -1.
*/
static int searchvar (FuncState *fs, TString *n, expdesc *var) {
  int i;
  for (i = fs->nactvar - 1; i >= 0; i--) {
    Vardesc *vd = getlocalvardesc(fs, i);
    if (eqstr(n, vd->vd.name)) {  /* found? */
      if (vd->vd.kind == RDKCTC)  /* compile-time constant? */
        init_exp(var, VCONST, fs->firstlocal + i);
      else if (vd->vd.kind == RDKENUM) {
        init_exp(var, VENUM, 0);
        var->u.ival = ivalue(&vd->k);
      }
      else  /* real variable */
        init_var(fs, var, i);
      return var->k;
    }
  }
  return -1;  /* not found */
}


/*
** Mark block where variable at given level was defined
** (to emit close instructions later).
*/
static void markupval (FuncState *fs, int level) {
  BlockCnt *bl = fs->bl;
  while (bl->nactvar > level)
    bl = bl->previous;
  bl->upval = 1;
  fs->needclose = 1;
}


/*
** Mark that current block has a to-be-closed variable.
*/
static void marktobeclosed (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  bl->upval = 1;
  bl->insidetbc = 1;
  fs->needclose = 1;
}


/*
** Find a variable with the given name 'n'. If it is an upvalue, add
** this upvalue into all intermediate functions. If it is a global, set
** 'var' as 'void' as a flag.
*/
static void singlevaraux (FuncState *fs, TString *n, expdesc *var, int base) {
  if (fs == NULL)  /* no more levels? */
    init_exp(var, VVOID, 0);  /* default is global */
  else {
    int v = searchvar(fs, n, var);  /* look up locals at current level */
    if (v >= 0) {  /* found? */
      if (v == VLOCAL && !base)
        markupval(fs, var->u.var.vidx);  /* local will be used as an upval */
    }
    else {  /* not found as local at current level; try upvalues */
      int idx = searchupvalue(fs, n);  /* try existing upvalues */
      if (idx < 0) {  /* not found? */
        singlevaraux(fs->prev, n, var, 0);  /* try upper levels */
        if (var->k == VLOCAL || var->k == VUPVAL)  /* local or upvalue? */
          idx  = newupvalue(fs, n, var);  /* will be a new upvalue */
        else  /* it is a global or a constant */
          return;  /* don't need to do anything at this level */
      }
      init_exp(var, VUPVAL, idx);  /* new or old upvalue */
    }
  }
}


inline int gett(LexState *ls) {
  return ls->t.token;
}


/*
** Adjust the number of results from an expression list 'e' with 'nexps'
** expressions to 'nvars' values.
*/
static void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e) {
  FuncState *fs = ls->fs;
  int needed = nvars - nexps;  /* extra values needed */
  if (hasmultret(e->k) || e->k == VSAFECALL) {  /* last expression has multiple returns? */
    int extra = needed + 1;  /* discount last expression itself */
    if (extra < 0)
      extra = 0;
    luaK_setreturns(fs, e, extra);  /* last exp. provides the difference */
  }
  else {
    if (e->k != VVOID)  /* at least one expression? */
      luaK_exp2nextreg(fs, e);  /* close last expression */
    if (needed > 0)  /* missing values? */
      luaK_nil(fs, fs->freereg, needed);  /* complete with nils */
  }
  if (needed > 0)
    luaK_reserveregs(fs, needed);  /* registers for extra values */
  else  /* adding 'needed' is actually a subtraction */
    fs->freereg += needed;  /* remove extra values */
}


/*
** Find a variable with the given name 'n', handling global variables
** too.
*/
static void singlevarinner (LexState *ls, TString *varname, expdesc *var, bool localonly = false) {
  FuncState *fs = ls->fs;
  singlevaraux(fs, varname, var, 1);
  if (var->k == VVOID && !localonly) {  /* global name? */
    expdesc key;
    singlevaraux(fs, ls->envn, var, 1);  /* get environment variable */
    lua_assert(var->k != VVOID);  /* this one must exist */
    luaK_exp2anyregup(fs, var);  /* but could be a constant */
    codestring(&key, varname);  /* key is variable name */
    luaK_indexed(fs, var, &key);  /* env[varname] */
  }
}

static void singlevar (LexState *ls, expdesc *var, TString *varname, bool localonly = false) {
  singlevarinner(ls, varname, var, localonly);
}

static void singlevar (LexState *ls, expdesc *var) {
  TString *varname = str_checkname(ls);
  singlevar(ls, var, varname);
}


#define enterlevel(ls)	luaE_incCstack(ls->L)


#define leavelevel(ls) ((ls)->L->nCcalls--)


/*
** Generates an error that a goto jumps into the scope of some
** local variable.
*/
static l_noret jumpscopeerror (LexState *ls, Labeldesc *gt) {
  const char *varname = getstr(getlocalvardesc(ls->fs, gt->nactvar)->vd.name);
  const char *msg;
  if (!gt->special) {
    msg = luaO_pushfstring(ls->L, "<goto %s> at line %d jumps into the scope of local '%s'", getstr((TString*)gt->name), gt->line, varname);
  } else {
    BlockCnt* bt = (BlockCnt*)gt->name;
    const char *type = bt->type == BlockType::BT_BREAK ? "break" : bt->type == BlockType::BT_CONTINUE ? "continue" : "?";
    msg = luaO_pushfstring(ls->L, "%s at line %d jumps into the scope of local '%s'", type, gt->line, varname);
  }
  luaK_semerror(ls, msg);  /* raise the error */
}

static bool samelabelnames(Labeldesc* l1, Labeldesc* l2) {
  return l1->special==l2->special && (l1->special ? l1->name == l2->name : eqstr((TString*)l1->name, (TString*)l2->name));
}

/*
** Solves the goto at index 'g' to given 'label' and removes it
** from the list of pending gotos.
** If it jumps into the scope of some variable, raises an error.
*/
static void solvegoto (LexState *ls, int g, Labeldesc *label) {
  int i;
  Labellist *gl = &ls->dyd->gt;  /* list of gotos */
  Labeldesc *gt = &gl->arr[g];  /* goto to be resolved */
  lua_assert(samelabelnames(gt, label));
  if (l_unlikely(gt->nactvar < label->nactvar))  /* enter some scope? */
    jumpscopeerror(ls, gt);
  luaK_patchlist(ls->fs, gt->pc, label->pc);
  for (i = g; i < gl->n - 1; i++)  /* remove goto from pending list */
    gl->arr[i] = gl->arr[i + 1];
  gl->n--;
}


/*
** Search for an active label with the given name.
*/
static Labeldesc *findlabel (LexState *ls, TString *name) {
  int i;
  Dyndata *dyd = ls->dyd;
  /* check labels in current function for a match */
  for (i = ls->fs->firstlabel; i < dyd->label.n; i++) {
    Labeldesc *lb = &dyd->label.arr[i];
    if (!lb->special && eqstr((TString*)lb->name, name))  /* correct label? */
      return lb;
  }
  return NULL;  /* label not found */
}


/*
** Adds a new label/goto in the corresponding list.
*/
static int newlabelentry (LexState *ls, Labellist *l, void *name,
                          int line, int pc, bool special) {
  int n = l->n;
  luaM_growvector(ls->L, l->arr, n, l->size,
                  Labeldesc, SHRT_MAX, "labels/gotos");
  l->arr[n].name = name;
  l->arr[n].line = line;
  l->arr[n].nactvar = ls->fs->nactvar;
  l->arr[n].close = 0;
  l->arr[n].special = special;
  l->arr[n].pc = pc;
  l->n = n + 1;
  return n;
}


static int newgotoentry (LexState *ls, void *name, int line, int pc, bool special) {
  return newlabelentry(ls, &ls->dyd->gt, name, line, pc, special);
}


/*
** Solves forward jumps. Check whether new label 'lb' matches any
** pending gotos in current block and solves them. Return true
** if any of the gotos need to close upvalues.
*/
static int solvegotos (LexState *ls, Labeldesc *lb) {
  Labellist *gl = &ls->dyd->gt;
  int i = ls->fs->bl->firstgoto;
  int needsclose = 0;
  while (i < gl->n) {
    if (samelabelnames(&gl->arr[i], lb)) {
      needsclose |= gl->arr[i].close;
      solvegoto(ls, i, lb);  /* will remove 'i' from the list */
    }
    else
      i++;
  }
  return needsclose;
}


/*
** Create a new label with the given 'name' at the given 'line'.
** 'last' tells whether label is the last non-op statement in its
** block. Solves all pending gotos to this new label and adds
** a close instruction if necessary.
** Returns true iff it added a close instruction.
*/
static int createlabel (LexState *ls, void *name, int line,
                        int last, bool special) {
  FuncState *fs = ls->fs;
  Labellist *ll = &ls->dyd->label;
  int l = newlabelentry(ls, ll, name, line, luaK_getlabel(fs), special);
  if (last) {  /* label is last no-op statement in the block? */
    /* assume that locals are already out of scope */
    ll->arr[l].nactvar = fs->bl->nactvar;
  }
  if (solvegotos(ls, &ll->arr[l])) {  /* need close? */
    luaK_codeABC(fs, OP_CLOSE, luaY_nvarstack(fs), 0, 0);
    return 1;
  }
  return 0;
}


/*
** Adjust pending gotos to outer level of a block.
*/
static void movegotosout (FuncState *fs, BlockCnt *bl) {
  int i;
  Labellist *gl = &fs->ls->dyd->gt;
  /* correct pending gotos to current block */
  for (i = bl->firstgoto; i < gl->n; i++) {  /* for each pending goto */
    Labeldesc *gt = &gl->arr[i];
    /* leaving a variable scope? */
    if (reglevel(fs, gt->nactvar) > reglevel(fs, bl->nactvar))
      gt->close |= bl->upval;  /* jump may need a close */
    gt->nactvar = bl->nactvar;  /* update goto level */
  }
}


static void enterblock (FuncState *fs, BlockCnt *bl, BlockType type) {
  bl->type = type;
  bl->nactvar = fs->nactvar;
  bl->firstlabel = fs->ls->dyd->label.n;
  bl->firstgoto = fs->ls->dyd->gt.n;
  bl->upval = 0;
  bl->insidetbc = static_cast<lu_byte>(fs->bl != NULL && fs->bl->insidetbc);
  bl->previous = fs->bl;
  fs->bl = bl;
  lua_assert(fs->freereg == luaY_nvarstack(fs));
}


/*
** generates an error for an undefined 'goto'.
*/
static l_noret undefgoto (LexState *ls, Labeldesc *gt) {
  lua_assert(!gt->special); // Should be checked by lbreak & continuestat before creating the label
  const char *msg = "no visible label '%s' for <goto> at line %d";
  msg = luaO_pushfstring(ls->L, msg, getstr((TString*)gt->name), gt->line);
  ls->setLineNumber(gt->line);
  luaK_semerror(ls, msg);
}


static void leaveblock (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  LexState *ls = fs->ls;
  int hasclose = 0;
  int stklevel = reglevel(fs, bl->nactvar);  /* level outside the block */
  removevars(fs, bl->nactvar);  /* remove block locals */
  lua_assert(bl->nactvar == fs->nactvar);  /* back to level on entry */
  if (bl->type != BlockType::BT_DEFAULT)  /* has to fix pending breaks / continues? */
    hasclose = createlabel(ls, bl, 0, 0, true);
  if (!hasclose && bl->previous && bl->upval)  /* still need a 'close'? */
    luaK_codeABC(fs, OP_CLOSE, stklevel, 0, 0);
  fs->freereg = stklevel;  /* free registers */
  ls->dyd->label.n = bl->firstlabel;  /* remove local labels */
  fs->bl = bl->previous;  /* current block now is previous one */
  if (bl->previous)  /* was it a nested block? */
    movegotosout(fs, bl);  /* update pending gotos to enclosing block */
  else {
    if (bl->firstgoto < ls->dyd->gt.n)  /* still pending gotos? */
      undefgoto(ls, &ls->dyd->gt.arr[bl->firstgoto]);  /* error */
  }
  ls->laststat.token = TK_EOS;  /* Prevent unreachable code warnings on blocks that don't explicitly check for TK_END. */
}


/*
** adds a new prototype into list of prototypes
*/
static Proto *addprototype (LexState *ls) {
  Proto *clp;
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;  /* prototype of current function */
  if (fs->np >= f->sizep) {
    int oldsize = f->sizep;
    luaM_growvector(L, f->p, fs->np, f->sizep, Proto *, MAXARG_Bx, "functions");
    while (oldsize < f->sizep)
      f->p[oldsize++] = NULL;
  }
  f->p[fs->np++] = clp = luaF_newproto(L);
  luaC_objbarrier(L, f, clp);
  return clp;
}


/*
** codes instruction to create new closure in parent function.
** The OP_CLOSURE instruction uses the last available register,
** so that, if it invokes the GC, the GC knows which registers
** are in use at that time.

*/
static void codeclosure (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs->prev;
  init_exp(v, VRELOC, luaK_codeABx(fs, OP_CLOSURE, 0, fs->np - 1));
  luaK_exp2nextreg(fs, v);  /* fix it at the last register */
}


static void open_func (LexState *ls, FuncState *fs, BlockCnt *bl) {
  Proto *f = fs->f;
  fs->prev = ls->fs;  /* linked list of funcstates */
  fs->ls = ls;
  ls->fs = fs;
  fs->pc = 0;
  fs->previousline = f->linedefined;
  fs->iwthabs = 0;
  fs->lasttarget = 0;
  fs->freereg = 0;
  fs->nk = 0;
  fs->nabslineinfo = 0;
  fs->np = 0;
  fs->nups = 0;
  fs->ndebugvars = 0;
  fs->nactvar = 0;
  fs->needclose = 0;
  fs->istrybody = 0;
  fs->seenrets = 0;
  fs->firstlocal = ls->dyd->actvar.n;
  fs->firstlabel = ls->dyd->label.n;
  fs->bl = NULL;
  fs->pinnedreg = -1;
  f->source = ls->source;
  luaC_objbarrier(ls->L, f, f->source);
  f->maxstacksize = 2;  /* registers 0/1 are always valid */
  enterblock(fs, bl, BlockType::BT_DEFAULT);
}


static void close_func (LexState *ls) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  luaK_ret(fs, luaY_nvarstack(fs), 0);  /* final return */
  leaveblock(fs);
  lua_assert(fs->bl == NULL);
  luaK_finish(fs);
  luaM_shrinkvector(L, f->code, f->sizecode, fs->pc, Instruction);
  luaM_shrinkvector(L, f->lineinfo, f->sizelineinfo, fs->pc, ls_byte);
  luaM_shrinkvector(L, f->abslineinfo, f->sizeabslineinfo,
                       fs->nabslineinfo, AbsLineInfo);
  luaM_shrinkvector(L, f->k, f->sizek, fs->nk, TValue);
  luaM_shrinkvector(L, f->p, f->sizep, fs->np, Proto *);
  luaM_shrinkvector(L, f->locvars, f->sizelocvars, fs->ndebugvars, LocVar);
  luaM_shrinkvector(L, f->upvalues, f->sizeupvalues, fs->nups, Upvaldesc);
  ls->fs = fs->prev;
  luaC_checkGC(L);
}



/*============================================================*/
/* GRAMMAR RULES */
/*============================================================*/


/*
** check whether current token is in the follow set of a block.
** 'until' closes syntactical blocks, but do not close scope,
** so it is handled in separate.
*/
static int block_follow (LexState *ls, int withuntil) {
  switch (ls->t.token) {
    case '$': {
      int ret;
      luaX_next(ls);
      ret = block_follow(ls, withuntil);
      luaX_prev(ls);
      return ret;
    }
    case TK_ELSE: case TK_ELSEIF:
    case TK_END: case TK_EOS:
      return 1;
    case TK_CATCH: case TK_PCATCH:
      return ls->used_try;
    case TK_UNTIL: return withuntil;
    default: return 0;
  }
}


static void newtable (LexState *ls, expdesc *v, const std::function<bool(expdesc *key, expdesc *val)>& gen);
static void statlist (LexState *ls, TypeHint *prop = nullptr, bool no_ret_implies_void = false) {
  /* statlist -> { stat [';'] } */
  bool ret = false;
  while (!block_follow(ls, 1)) {
    ret = (ls->t.token == TK_RETURN);
    TypeHint p{};
#if defined LUAI_ASSERT
    const auto levels = ls->L->nCcalls;
#endif
    statement(ls, &p);
    lua_assert(levels == ls->L->nCcalls);
    if (prop && /* do we need to propagate the return type? */
        !p.empty()) { /* is there a return path here? */
      prop->merge(p);
    }
    if (ret) break;
  }
  if (prop) { /* do we need to propagate the return type? */
    if (!ret && /* had no return statement? */
        no_ret_implies_void) { /* does that imply a void return? */
      prop->emplaceTypeDesc(VT_VOID); /* propagate */
    }
    prop->fixTypes();
  }
  if (!ls->fs->bl->export_symbols.empty()) {
    if (ret) {
      luaX_prev(ls);
      luaX_syntaxerror(ls, "'export' used but body already returns something");
    }
    enterlevel(ls);
    size_t i = 0;
    expdesc t;
    if (ls->fs->istrybody) {
      init_exp(&t, VKINT, 0);
      t.u.ival = 1;
      luaK_exp2nextreg(ls->fs, &t);
    }
    newtable(ls, &t, [ls, &i](expdesc *k, expdesc *v) {
      if (i == ls->fs->bl->export_symbols.size())
        return false;
      codestring(k, ls->fs->bl->export_symbols.at(i));
      singlevar(ls, v, ls->fs->bl->export_symbols.at(i));
      ++i;
      return true;
    });
    ls->fs->seenrets = 1<<1;
    luaK_ret(ls->fs, luaK_exp2anyreg(ls->fs, &t)-ls->fs->istrybody, 1+ls->fs->istrybody);
    leavelevel(ls);
    ls->fs->bl->export_symbols.clear();
  }
}

static BlockCnt* searchloop(FuncState* fs, BlockType type, lua_Integer depth) {
  for(;;) {
    BlockCnt* bl = fs->bl;
    while (bl) {
      if (bl->type == type && --depth == 0) return bl;
      bl = bl->previous;
    }
    if (!fs->istrybody)
      return 0;
    fs = fs->prev;
  }
}

static int countdepth(FuncState* fs, BlockType type) {
  int depth = 0;
  for(;;) {
    BlockCnt* bl = fs->bl;
    while (bl) {
      if (bl->type == type) depth++;
      bl = bl->previous;
    }
    if (!fs->istrybody)
      return depth;
    fs = fs->prev;
  }
}

/*
** Continue statement. Semantically similar to "goto continue".
** Unlike break, this doesn't use labels. It tracks where to jump via BlockCnt.scopeend;
*/
static void continuestat (LexState *ls) {
  auto line = ls->getLineNumber();
  FuncState *fs = ls->fs;
  luaX_next(ls);  /* skip 'continue' */
  lua_Integer backwards = 1;
  if (ls->t.token == TK_INT) {
    backwards = ls->t.seminfo.i;
    if (backwards == 0) {
      throwerr(ls, "expected number of blocks to skip, found '0'", "unexpected '0'", line);
    }
    luaX_next(ls);
  }
  BlockCnt* l = searchloop(fs, BlockType::BT_CONTINUE, backwards);
  if (l) {
    newgotoentry(ls, l, line, luaK_jump(fs), true);
  }
  else {
    int foundloops = countdepth(fs, BlockType::BT_CONTINUE);
    if (foundloops == 0)
      throwerr(ls, "'continue' outside of a loop","'continue' can only be used inside the context of a loop.", line);
    else {
      if (foundloops == 1) {
        throwerr(ls,
          "'continue' argument exceeds the amount of enclosing loops",
            luaO_fmt(ls->L, "there is only 1 enclosing loop.", foundloops));
      }
      else {
        throwerr(ls,
          "'continue' argument exceeds the amount of enclosing loops",
            luaO_fmt(ls->L, "there are only %d enclosing loops.", foundloops));
      }
    }
  }
}


static void lbreak (LexState *ls, lua_Integer backwards, int line, int jump) {
  BlockCnt* l = searchloop(ls->fs, BlockType::BT_BREAK, backwards);
  if (l) {
    newgotoentry(ls, l, line, jump, true);
  }
  else {
    throwerr(ls, "break can't skip that many blocks", "try a smaller number", line);
  }
}


/*
** Break statement. Very similiar to `continue` usage, but it jumps slightly more forward.
**
** Implementation Detail:
**   Unlike normal Lua, it has been reverted from a label implementation back into a mix between a label & patchlist implementation.
**   This allows reusage of the existing "continue" implementation, which has been time-tested extensively by now.
*/
static void breakstat (LexState *ls) {
  const auto line = ls->getLineNumber();
  luaX_next(ls); /* skip TK_BREAK */
  lua_Integer backwards = 1;
  if (ls->t.token == TK_INT) {
    backwards = ls->t.seminfo.i;
    if (backwards == 0) {
      throwerr(ls, "expected number of blocks to skip, found '0'", "unexpected '0'", line);
    }
    luaX_next(ls);
  }
  lbreak(ls, backwards, line, luaK_jump(ls->fs));
}


static void fieldsel (LexState *ls, expdesc *v) {
  /* fieldsel -> ['.' | ':'] NAME */
  FuncState *fs = ls->fs;
  expdesc key;
  luaK_exp2anyregup(fs, v);
  luaX_next(ls);  /* skip the dot or colon */
  codename(ls, &key, N_RESERVED);
  luaK_indexed(fs, v, &key);
}


static void yindex (LexState *ls, expdesc *v) {
  /* index -> '[' expr ']' */
  luaX_next(ls);  /* skip the '[' */
  expr(ls, v);
  luaK_exp2val(ls->fs, v);
  checknext(ls, ']');
}


/*
** {======================================================================
** Rules for Constructors
** =======================================================================
*/


typedef struct ConsControl {
  expdesc v;  /* last list item read */
  expdesc *t;  /* table descriptor */
  int nh;  /* total number of 'record' elements */
  int na;  /* number of array elements already stored */
  int tostore;  /* number of array elements pending to be stored */
} ConsControl;


static void preassignfield (LexState *ls, expdesc& key) {
  if (key.k == VKSTR) {
    if (ls->constructorfieldsets.top().count(key.u.strval)) {
      throw_warn(ls, "duplicate table field", luaO_fmt(ls->L, "this overwrites the value assigned to '%s' field earlier", getstr(key.u.strval)), WT_FIELD_SHADOW);
      lua_pop(ls->L, 1);
    }
    else
      ls->constructorfieldsets.top().emplace(key.u.strval);
  }
}

static void recfield (LexState *ls, ConsControl *cc, bool for_class) {
  /* recfield -> (NAME | '['exp']') = exp */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  if (ls->t.token == TK_NAME) {
    checklimit(fs, cc->nh, MAX_INT, "items in a constructor");
    TString *name = str_checkname(ls, N_RESERVED);  /* we already know this is a TK_NAME, but don't wanna raise non-portable-name, so passing N_RESERVED */
    if (for_class) {
      if (strcmp(getstr(name), "public") == 0) {
        name = str_checkname(ls);
      }
      else if (strcmp(getstr(name), "protected") == 0) {
        luaX_syntaxerror(ls, "'protected' is reserved in this context");
        name = str_checkname(ls);
      }
      else if (strcmp(getstr(name), "private") == 0) {
        const auto field_name = ls->classes.top().addPrefix(str_checkname(ls)->toCpp());
        name = luaX_newstring(ls, field_name.c_str());
      }
    }
    codestring(&key, name);
  }
  else  /* ls->t.token == '[' */
    yindex(ls, &key);
  if (for_class)
    UNUSED(gettypehint(ls));
  cc->nh++;
  tab = *cc->t;
  preassignfield(ls, key);
  luaK_indexed(fs, &tab, &key);
  if (for_class) {
    if (testnext(ls, '='))
      expr(ls, &val);
    else
      init_exp(&val, VNIL, 0);
  }
  else {
    checknext(ls, '=');
    expr(ls, &val);
  }
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;  /* free registers */
}

static void prenamedfield (LexState *ls, ConsControl *cc, const char *name) {
  FuncState* fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  codestring(&key, luaX_newstring(ls, name));
  cc->nh++;
  luaX_next(ls); /* skip name token */
  checknext(ls, '=');
  tab = *cc->t;
  preassignfield(ls, key);
  luaK_indexed(fs, &tab, &key);
  expr(ls, &val);
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;  /* free registers */
}

static void closelistfield (FuncState *fs, ConsControl *cc) {
  if (cc->v.k == VVOID) return;  /* there is no list item */
  luaK_exp2nextreg(fs, &cc->v);
  cc->v.k = VVOID;
  if (cc->tostore == LFIELDS_PER_FLUSH) {
    luaK_setlist(fs, cc->t->u.reg, cc->na, cc->tostore);  /* flush */
    cc->na += cc->tostore;
    cc->tostore = 0;  /* no more items pending */
  }
}


static void lastlistfield (FuncState *fs, ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (hasmultret(cc->v.k)) {
    luaK_setmultret(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.reg, cc->na, LUA_MULTRET);
    cc->na--;  /* do not count last expression (unknown number of elements) */
  }
  else {
    if (cc->v.k != VVOID)
      luaK_exp2nextreg(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.reg, cc->na, cc->tostore);
  }
  cc->na += cc->tostore;
}


static void listfield (LexState *ls, ConsControl *cc) {
  /* listfield -> exp */
  expr(ls, &cc->v);
  cc->tostore++;
}


static void body (LexState *ls, expdesc *e, int ismethod, int line, TypeDesc *funcdesc = nullptr);
static void funcfield (LexState *ls, struct ConsControl *cc, int ismethod, bool isprivate = false) {
  /* funcfield -> function NAME funcargs */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  cc->nh++;
  luaX_next(ls); /* skip TK_FUNCTION */
  codename(ls, &key);
  if (isprivate) {
    const auto new_name = ls->classes.top().addPrefix(getstr(key.u.strval));
    codestring(&key, luaX_newstring(ls, new_name.c_str()));
  }
  if (ismethod)
    ismethod += (strcmp(getstr(key.u.strval), "__construct") == 0);
  tab = *cc->t;
  luaK_indexed(fs, &tab, &key);
  body(ls, &val, ismethod, ls->getLineNumber());
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;  /* free registers */
}


static void field (LexState *ls, ConsControl *cc, bool for_class = false) {
  /* field -> listfield | recfield | funcfield */
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", "function");
    ss.push("stat", "static");
  }
#endif
  if (ls->t.IsReserved() && luaX_lookahead(ls) == '=') {
    prenamedfield(ls, cc, luaX_reserved2str(ls->t.token));
  }
  else switch(ls->t.token) {
    case TK_NAME: {  /* may be 'listfield', 'recfield' or static 'funcfield' */
      if (strcmp(getstr(ls->t.seminfo.ts), "static") == 0) {
        luaX_next(ls);
        check(ls, TK_FUNCTION);
        funcfield(ls, cc, false);
      }
      else if (for_class && luaX_lookahead(ls) == TK_FUNCTION && strcmp(getstr(ls->t.seminfo.ts), "public") == 0) {
        luaX_next(ls);
        funcfield(ls, cc, true);
      }
      else if (for_class && luaX_lookahead(ls) == TK_FUNCTION && strcmp(getstr(ls->t.seminfo.ts), "private") == 0) {
        luaX_next(ls);
        funcfield(ls, cc, true, true);
      }
      else {
        if (!for_class && luaX_lookahead(ls) != '=')  /* expression? */
          listfield(ls, cc);
        else
          recfield(ls, cc, for_class);
      }
      break;
    }
    case '[': {
      recfield(ls, cc, for_class);
      break;
    }
    case TK_FUNCTION: {
      if (luaX_lookahead(ls) == '(') {
        listfield(ls, cc);
      }
      else {
        funcfield(ls, cc, true);
      }
      break;
    }
    default: {
      if (for_class)
        throwerr(ls, luaO_fmt(ls->L, "unexpected token: %s", luaX_token2str(ls, ls->t)), "expected a class member");
      listfield(ls, cc);
      break;
    }
  }
}


static void constructor (LexState *ls, expdesc *t) {
  /* constructor -> '{' [ field { sep field } [sep] ] '}'
     sep -> ',' | ';' */
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  ls->constructorfieldsets.emplace();
  ConsControl cc;
  luaK_code(fs, 0);  /* space for extra arg. */
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VNONRELOC, fs->freereg);  /* table will be at stack top */
  luaK_reserveregs(fs, 1);
  init_exp(&cc.v, VVOID, 0);  /* no value (yet) */
  checknext(ls, '{');
  do {
    lua_assert(cc.v.k == VVOID || cc.tostore > 0);
    if (ls->t.token == '}') break;
    closelistfield(fs, &cc);
    field(ls, &cc);
  } while (testnext(ls, ',') || testnext(ls, ';'));
  if (ls->t.token == TK_NAME || ls->t.token == TK_FUNCTION || ls->t.token == '[' || ls->t.isSimple()) {
    check_match(ls, '}', '{', line, "Ensure that you've delimited the previous field with ',' or ';'.");
  }
  else {
    check_match(ls, '}', '{', line);
  }
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t->u.reg, cc.na, cc.nh);
  ls->constructorfieldsets.pop();
}


static void newtable (LexState *ls, expdesc *v, const std::function<bool(expdesc*)>& gen) {
  FuncState* fs = ls->fs;
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  ConsControl cc;
  luaK_code(fs, 0);  /* space for extra arg. */
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = v;
  init_exp(v, VNONRELOC, fs->freereg);  /* table will be at stack top */
  luaK_reserveregs(fs, 1);
  while (gen(&cc.v)) {
    ++cc.tostore;
    closelistfield(fs, &cc);
  }
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, v->u.reg, cc.na, cc.nh);
}

static void newtable (LexState *ls, expdesc *v, const std::function<bool(expdesc *key, expdesc *val)>& gen) {
  FuncState* fs = ls->fs;
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  ConsControl cc;
  luaK_code(fs, 0);  /* space for extra arg. */
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = v;
  init_exp(v, VNONRELOC, fs->freereg);  /* table will be at stack top */
  luaK_reserveregs(fs, 1);
  init_exp(&cc.v, VVOID, 0);
  while (true) {
    closelistfield(fs, &cc);
    int reg = ls->fs->freereg;
    expdesc tab, key, val;
    if (!gen(&key, &val))
      break;
    luaK_exp2val(ls->fs, &key);
    cc.nh++;
    tab = *cc.t;
    luaK_indexed(fs, &tab, &key);
    luaK_storevar(fs, &tab, &val);
    fs->freereg = reg;
  }
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, v->u.reg, cc.na, cc.nh);
}


static void classname (LexState *ls, expdesc *v) {
  singlevarinner(ls, str_checkname(ls, 0), v);
  while (ls->t.token == '.')
    fieldsel(ls, v);
}

static void skip_classname (LexState *ls) {
  str_checkname(ls, 0);
  while (ls->t.token == '.') {
    luaX_next(ls);
    str_checkname(ls, N_RESERVED);
  }
}

static size_t checkextends (LexState *ls) {
  size_t pos = 0;
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", "extends");
    ss.push("stat", "function");
    ss.push("stat", "static");
    ss.push("stat", "end");
  }
#endif
  if (ls->t.token == TK_EXTENDS) {
    luaX_next(ls);
    pos = luaX_getpos(ls);
    skip_classname(ls);
  }
  ls->classes.top().parent_name_pos = pos;
  return pos;
}

static void applyextends (LexState *ls, size_t name_pos, size_t parent_pos, int line) {
  FuncState *fs = ls->fs;

  expdesc f;
  singlevaraux(fs, luaS_newliteral(ls->L, "Pluto_operator_extends"), &f, 1);
  lua_assert(f.k != VVOID);
  luaK_exp2nextreg(fs, &f);

  expdesc args;
  auto pos = luaX_getpos(ls);
  luaX_setpos(ls, name_pos);
  classname(ls, &args);
  luaK_exp2nextreg(fs, &args);
  luaX_setpos(ls, parent_pos);
  classname(ls, &args);
  luaX_setpos(ls, pos);

  lua_assert(f.k == VNONRELOC);
  int base = f.u.reg;  /* base register for call */
  luaK_exp2nextreg(fs, &args);  /* close last argument */
  int nparams = fs->freereg - (base + 1);
  init_exp(&f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base + 1;
}

static size_t preprocessclass (LexState *ls) {
  int allowed_ends = 0;
  bool expect_block_opener = false;
  const auto start = luaX_getpos(ls);

  while (ls->t.token != TK_EOS) {
    if (ls->t.token == TK_END && allowed_ends-- <= 0) {
      // printf("Preprocessed class body ending at line %d.\n", ls->getLineNumber());
      break;
    }

    // This is only checking *inside* our current class body, the parser has already skipped the class declaration.
    switch (ls->t.normalizedToken()) {
    case TK_CLASS:  /* 'class' or 'enum class' */
    case TK_IF:
    case TK_CATCH:
      expect_block_opener = true;
      ++allowed_ends;
      break;

    case TK_FUNCTION:
      expect_block_opener = false;
      ++allowed_ends;
      break;

    case TK_THEN:
    case TK_BEGIN:
      expect_block_opener = false;
      break;

    case TK_DO:
      if (expect_block_opener)
        expect_block_opener = false;
      else
        ++allowed_ends;  /* forstat, whilestat, switchstat, dostat, '-> do' */
      break;

    case TK_ENUM:
      expect_block_opener = true;
      if (luaX_lookahead(ls) != TK_CLASS) {  /* 'enum class' would already be counted */
        ++allowed_ends;
      }
      break;
    }

    if (ls->t.token == TK_NAME && strcmp(getstr(ls->t.seminfo.ts), "private") == 0) {
      if (luaX_lookahead(ls) == TK_FUNCTION) {
        checknext(ls, TK_NAME);
        checknext(ls, TK_FUNCTION);
        ls->classes.top().addField(getstr(ls->t.seminfo.ts));
        ++allowed_ends; // For TK_FUNCTION
      }
      else if (luaX_lookahead(ls) == TK_NAME) {
        checknext(ls, TK_NAME);
        ls->classes.top().addField(getstr(ls->t.seminfo.ts));
      }
    }

    luaX_next(ls);
  }

  const auto finish = luaX_getpos(ls);
  luaX_setpos(ls, start);
  return finish;
}

static void classexpr (LexState *ls, expdesc *t) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();
  testnext2(ls, TK_BEGIN, TK_DO);
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  ls->constructorfieldsets.emplace();
  ConsControl cc;
  luaK_code(fs, 0);  /* space for extra arg. */
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VNONRELOC, fs->freereg);  /* table will be at stack top */
  luaK_reserveregs(fs, 1);
  init_exp(&cc.v, VVOID, 0);  /* no value (yet) */
  const auto finish = preprocessclass(ls);
  while (ls->t.token != TK_END) {
    lua_assert(cc.v.k == VVOID || cc.tostore > 0);
    closelistfield(fs, &cc);
    field(ls, &cc, true);
    (testnext(ls, ',') || testnext(ls, ';'));
  }
  if (finish != luaX_getpos(ls)) { // The preprocessor should've terminated at the same exact spot.
    throwerr(ls,
      "internal error: 'preprocessclass' failed to handle a block",
      "",
      "Report this at: https://github.com/PlutoLang/Pluto/issues"
    );
  }
  check_match(ls, TK_END, TK_CLASS, line);
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t->u.reg, cc.na, cc.nh);
  ls->constructorfieldsets.pop();
}


static void check_assignment (LexState *ls, const expdesc *v) {
  if (v->k == VINDEXUP && ls->isKeywordEnabled(TK_GLOBAL)) {
    luaX_prev(ls);
    TString *name = str_checkname(ls, N_RESERVED_NON_VALUE | N_OVERRIDABLE);
    if (ls->explicit_globals.count(name) == 0) {
      throw_warn(ls, "implicit global creation", "prefix this with 'global' to be explicit", WT_IMPLICIT_GLOBAL);
    }
  }
}


static void classstat (LexState *ls, int line, const bool global) {
  ls->classes.emplace();

  size_t name_pos = luaX_getpos(ls);
  expdesc v;
  if (global) {
    singlevarinner(ls, str_checkname(ls, 0), &v);
  }
  else {
    classname(ls, &v);
    check_assignment(ls, &v);
  }

  size_t parent_pos = checkextends(ls);

  expdesc t;
  classexpr(ls, &t);
  check_readonly(ls, &v);
  luaK_storevar(ls->fs, &v, &t);
  luaK_fixline(ls->fs, line);

  lua_assert(ls->getParentClassPos() == parent_pos);
  ls->classes.pop();

  if (parent_pos)
    applyextends(ls, name_pos, parent_pos, line);
}


static void localclass (LexState *ls) {
  luaX_prev(ls);
  check_for_non_portable_code(ls);
  luaX_next(ls);
  auto line = ls->getLineNumber();

  ls->classes.emplace();

  size_t name_pos = luaX_getpos(ls);
  TString *name = str_checkname(ls, 0);
  size_t parent_pos = checkextends(ls);

  new_localvar(ls, name, line);

  expdesc t;
  classexpr(ls, &t);

  adjust_assign(ls, 1, 1, &t);
  adjustlocalvars(ls, 1);

  lua_assert(ls->getParentClassPos() == parent_pos);
  ls->classes.pop();

  if (parent_pos)
    applyextends(ls, name_pos, parent_pos, line);
}

/* }====================================================================== */


static void setvararg (FuncState *fs, int nparams) {
  fs->f->is_vararg = 1;
  luaK_codeABC(fs, OP_VARARGPREP, nparams, 0, 0);
}


enum expflags {
  E_NO_METHOD_CALL   = 1 << 0,  /* ':' may not be used to initiate a method call */
  E_NO_CALL          = 1 << 1,
  E_NO_BOR           = 1 << 2,
  E_PIPERHS          = 1 << 3,
  E_WALRUS           = 1 << 4,
  E_OR_KILLED_WALRUS = 1 << 5,
  E_NO_CONSUME_COLON = 1 << 6,  /* this expression must not consume a non-nested colon */
};

static void simpleexp (LexState *ls, expdesc *v, int flags = 0, TypeHint *prop = nullptr);


/* keep advancing until we hit `token` */
static void skip_until (LexState *ls, int token) {
  int parens = 0;
  int curlys = 0;
  while (ls->t.token != TK_EOS) {
    if (ls->t.token == '(') {
      parens++;
    }
    else if (ls->t.token == ')') {
      parens--;
    }
    else if (ls->t.token == '{') {
      curlys++;
    }
    else if (ls->t.token == '}') {
      curlys--;
    }
    else if (ls->t.token == token) {
      if (parens == 0 && curlys == 0) {
        break;
      }
    }
    luaX_next(ls);
  }
}

/* keep advancing until we hit ',' or non-nested ')' */
static void skip_over_simpleexp_within_parenlist (LexState *ls) {
  int parens = 0;
  int curlys = 0;
  while (ls->t.token != TK_EOS) {
    if (ls->t.token == '(') {
      parens++;
    }
    else if (ls->t.token == ')') {
      if (parens == 0 && curlys == 0) {
        break;
      }
      parens--;
    }
    else if (ls->t.token == '{') {
      curlys++;
    }
    else if (ls->t.token == '}') {
      curlys--;
    }
    else if (ls->t.token == ',') {
      if (parens == 0 && curlys == 0) {
        break;
      }
    }
    luaX_next(ls);
  }
}

/* keep advancing until we hit ',' or '|' */
static void skip_over_simpleexp_within_lambdaparlist (LexState *ls) {
  int parens = 0;
  int curlys = 0;
  while (ls->t.token != TK_EOS) {
    if (ls->t.token == '(') {
      parens++;
    }
    else if (ls->t.token == ')') {
      parens--;
    }
    else if (ls->t.token == '{') {
      curlys++;
    }
    else if (ls->t.token == '}') {
      curlys--;
    }
    else if (ls->t.token == ',' || ls->t.token == '|') {
      if (parens == 0 && curlys == 0) {
        break;
      }
    }
    luaX_next(ls);
  }
}

static void parlist (LexState *ls, std::vector<std::pair<TString*, TString*>>* promotions, std::vector<size_t>* fallbacks, TString** varargname, bool lambda) {
  /* parlist -> [ {NAME ','} (NAME | '...') ] */
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int nparams = 0;
  int isvararg = 0;
  if (ls->t.token != ')' && ls->t.token != '|') {  /* is 'parlist' not empty? */
    do {
      if (isnametkn(ls, N_OVERRIDABLE) || (ls->t.IsNonCompatible() && trydisablekeyword(ls) && isnametkn(ls, N_OVERRIDABLE))) {
        auto parname = str_checkname(ls, N_OVERRIDABLE);
        if (promotions) {
          if (strcmp(getstr(parname), "public") == 0) {
            parname = str_checkname(ls, N_OVERRIDABLE);
            promotions->emplace_back(parname, parname);
          }
          else if (strcmp(getstr(parname), "protected") == 0) {
            luaX_syntaxerror(ls, "'protected' is reserved in this context");
          }
          else if (strcmp(getstr(parname), "private") == 0) {
            parname = str_checkname(ls, N_OVERRIDABLE);
            const auto field_name = ls->classes.top().addPrefix(parname->toCpp());
            promotions->emplace_back(parname, luaX_newstring(ls, field_name.c_str()));
          }
        }
        auto parhint = gettypehint(ls);
        auto vidx = new_localvar(ls, parname, parhint);
        *getlocalvardesc(fs, vidx)->vd.prop = parhint;  /* set hinted type as propagated type */
        if (fallbacks) {
          if (testnext(ls, '=')) {
            if (ls->t.token == TK_NIL) {
              throwerr(ls, "default argument expected", "nil is not a valid default argument");
            }
            fallbacks->emplace_back(luaX_getpos(ls));
            if (lambda)
              skip_over_simpleexp_within_lambdaparlist(ls);
            else
              skip_over_simpleexp_within_parenlist(ls);
          }
          else {
            fallbacks->emplace_back(0);
          }
        }
        nparams++;
      }
      else if (ls->t.token == TK_DOTS) {
        luaX_next(ls);
        isvararg = 1;
        if (varargname && ls->t.token == TK_NAME) {
          *varargname = ls->t.seminfo.ts;
          luaX_next(ls);
        }
      }
      else luaX_syntaxerror(ls, "<name> or '...' expected");
    } while (!isvararg && testnext(ls, ','));
  }
  adjustlocalvars(ls, nparams);
  f->numparams = cast_byte(fs->nactvar);
  if (isvararg)
    setvararg(fs, f->numparams);  /* declared vararg */
  luaK_reserveregs(fs, fs->nactvar);  /* reserve registers for parameters */
}


/* Returns true if the function is declared '<nodiscard>'. */
static bool getfunctionattribute (LexState *ls) {
  if (testnext(ls, '<')) {
    TString *ts = str_checkname(ls);
    const char *attr = getstr(ts);
    checknext(ls, '>');
    if (strcmp(attr, "nodiscard") == 0)
      return true;
    else {
      luaX_prev(ls); // back to '>'
      luaX_prev(ls); // back to attribute
      luaK_semerror(ls,
        luaO_pushfstring(ls->L, "unknown attribute '%s'", attr));
    }
  }
  return false;
}


static void defaultarguments (LexState *ls, int ismethod, const std::vector<size_t>& fallbacks, int flags = 0) {
  const auto saved_pos = luaX_getpos(ls);
  int fallback_idx = (ismethod ? 1 : 0);
  for (const auto& tidx : fallbacks) {
    if (tidx != 0) {
      enterlevel(ls);
      FuncState *fs = ls->fs;

      expdesc nilable;
      Vardesc *vd = getlocalvardesc(fs, fallback_idx);
      singlevaraux(fs, vd->vd.name, &nilable, 0);

      luaX_setpos(ls, tidx);
      expdesc nilexp;
      luaK_infix(fs, OPR_NE, &nilable);
      init_exp(&nilexp, VNIL, 0);
      luaK_posfix(fs, OPR_NE, &nilable, &nilexp, ls->getLineNumber());
      auto pc = nilable.u.pc;

      expdesc fallback;
      expr(ls, &fallback, nullptr, flags);
      luaK_setoneret(fs, &fallback);
      singlevaraux(fs, vd->vd.name, &nilable, 0);
      luaK_storevar(fs, &nilable, &fallback);

      luaK_patchtohere(fs, pc);
      leavelevel(ls);
    }
    ++fallback_idx;
  }
  luaX_setpos(ls, saved_pos);
}


static void namedvararg (LexState *ls, TString *varargname) {
  enterlevel(ls);
  luaX_prev(ls);  /* in case we need to raise a var-shadow warning, ensure we're on the right line */
  new_localvar(ls, varargname);
  luaX_next(ls);

  FuncState *fs = ls->fs;
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  luaK_code(fs, 0);
  expdesc t;
  init_exp(&t, VNONRELOC, fs->freereg);
  ConsControl cc;
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = &t;
  luaK_reserveregs(fs, 1);
  lua_assert(fs->f->is_vararg);
  init_exp(&cc.v, VVARARG, luaK_codeABC(fs, OP_VARARG, 0, 0, 1));
  cc.tostore++;
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t.u.reg, cc.na, cc.nh);

  adjust_assign(ls, 1, 1, &t);
  adjustlocalvars(ls, 1);
  leavelevel(ls);
}


static void checkrettype (LexState *ls, TypeHint& rethint, TypeHint& retprop, int line) {
  if (!rethint.empty() /* has type hint for return type? */
      && !retprop.empty() && retprop.descs[0].type != VT_DUNNO /* return type is known? */
      && !rethint.isCompatibleWith(retprop)) { /* incompatible? */
    std::string err = "function was hinted to return ";
    err.append(rethint.toString());
    err.append(" but actually returns ");
    err.append(retprop.toString());
    throw_warn(ls, err.c_str(), line, WT_TYPE_MISMATCH);
  }
}


static void propfuncdesc (LexState *ls, FuncState& new_fs, TypeHint& retprop, TypeDesc *funcdesc) {
  funcdesc->type = VT_FUNC;
  funcdesc->proto = new_fs.f;
  funcdesc->retn = new_typehint(ls);
  *funcdesc->retn = retprop;
  int vidx = new_fs.firstlocal;
  for (lu_byte i = 0; i != funcdesc->getNumTypedParams(); ++i) {
    funcdesc->params[i] = ls->dyd->actvar.arr[vidx].vd.hint;
    ++vidx;
  }
}


static void body (LexState *ls, expdesc *e, int ismethod, int line, TypeDesc *funcdesc) {
  /* body ->  '(' parlist ')' block END */
  ls->pushContext(PARCTX_BODY);
  FuncState new_fs;
  BlockCnt bl;
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = line;
  open_func(ls, &new_fs, &bl);
  checknext(ls, '(');
  if (ismethod) {
    new_localvarliteral(ls, "self");  /* create 'self' parameter */
    adjustlocalvars(ls, 1);
  }
  BodyState& bs = ls->bodystates.emplace();
  TString *varargname = nullptr;
  parlist(ls, (ismethod == 2 ? &bs.promotions : nullptr), &bs.fallbacks, &varargname, false);
  if (ls->t.token != ')' && ismethod != 2 && luaX_lookbehind(ls).token == TK_NAME) {
    const char *str = getstr(luaX_lookbehind(ls).seminfo.ts);
    if (strcmp(str, "private") == 0 || strcmp(str, "protected") == 0 || strcmp(str, "public") == 0) {
      throwerr(ls, luaO_fmt(ls->L, "attempt to use constructor promotion outside of '__construct' near %s", luaX_token2str(ls, ls->t)), "this is invalid syntax.");
    }
  }
  checknext(ls, ')');
  defaultarguments(ls, ismethod, bs.fallbacks);
  for (const auto& e : bs.promotions) {
    FuncState *fs = ls->fs;
    expdesc tab, key, val;

    /* self[field] */
    singlevaraux(fs, luaS_newliteral(ls->L, "self"), &tab, 0);
    init_exp(&key, VKSTR, 0);
    key.u.strval = e.second;
    luaK_indexed(fs, &tab, &key);

    /* ... = arg */
    singlevaraux(fs, e.first, &val, 0);
    luaK_storevar(fs, &tab, &val);
  }
  if (varargname)
    namedvararg(ls, varargname);
  TypeHint rethint = gettypehint(ls, true);
  const bool nodiscard = getfunctionattribute(ls);
  TypeHint retprop{};
  statlist(ls, &retprop, true);
  checkrettype(ls, rethint, retprop, line);
  if (funcdesc) {
    propfuncdesc(ls, new_fs, retprop, funcdesc);
    funcdesc->nodiscard = nodiscard;
  }
  new_fs.f->lastlinedefined = ls->getLineNumber();
  check_match(ls, TK_END, TK_FUNCTION, line);
  codeclosure(ls, e);
  close_func(ls);
  ls->popContext(PARCTX_BODY);
  ls->bodystates.pop();
}


/*
** Lambda implementation.
** Shorthands lambda expressions into `function (...) return ... end`.
** The '|' token was chosen because it's not commonly used as a unary operator in programming.
** The '->' arrow syntax looked more visually appealing than a colon. It also plays along with common lambda tokens.
*/
static void lambdabody (LexState *ls, expdesc *e, int line, TypeDesc *funcdesc = nullptr) {
  FuncState new_fs;
  BlockCnt bl;
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = line;
  open_func(ls, &new_fs, &bl);
  BodyState& bs = ls->bodystates.emplace();
  TString *varargname = nullptr;
  parlist(ls, nullptr, &bs.fallbacks, &varargname, true);
  checknext(ls, '|');
  TypeHint rethint = gettypehint(ls, true);
  const bool nodiscard = getfunctionattribute(ls);
  checknext(ls, TK_ARROW);
  defaultarguments(ls, 0, bs.fallbacks, E_NO_BOR);
  if (varargname)
    namedvararg(ls, varargname);
  TypeHint retprop{};
  if (testnext(ls, TK_DO)) {
    ls->pushContext(PARCTX_BODY);
    statlist(ls, &retprop, true);
    check_match(ls, TK_END, TK_ARROW, line);
    ls->popContext(PARCTX_BODY);
  }
  else {
    ls->pushContext(PARCTX_LAMBDA_BODY);
    expr(ls, e, &retprop);
    ls->fs->seenrets |= 1<<1;
    luaK_ret(&new_fs, luaK_exp2anyreg(&new_fs, e), 1);
    ls->popContext(PARCTX_LAMBDA_BODY);
  }
  checkrettype(ls, rethint, retprop, line);
  if (funcdesc) {
    propfuncdesc(ls, new_fs, retprop, funcdesc);
    funcdesc->nodiscard = nodiscard;
  }
  new_fs.f->lastlinedefined = ls->getLineNumber();
  codeclosure(ls, e);
  close_func(ls);
  ls->bodystates.pop();
}


static void expr_propagate (LexState *ls, expdesc *v, TypeHint& t) {
  expr(ls, v, &t);
  exp_propagate(ls, *v, t);
}


static void expr_propagate_warn (LexState *ls, expdesc *v, TypeHint& t, std::unordered_set<TString*>& names) {
  expr(ls, v, &t);
  exp_propagate(ls, *v, t);
  if (v->k == VINDEXUP) {
    TString *key = tsvalue(&ls->fs->f->k[v->u.ind.idx]);
    for (auto global : common_global_names) {
      if (strncmp(getstr(key), global, sizeof(global) - 1) == 0) {
        names.emplace(key);
        break;
      }
    }
  }
}


static void explist_nonlinear_arg (LexState *ls, expdesc *v, size_t tidx, TypeHint& t) {
  if (tidx == 0) {
    init_exp(v, VNIL, 0);
    t.emplaceTypeDesc(VT_NIL);
  }
  else {
    luaX_setpos(ls, tidx);
    expr_propagate(ls, v, t);
  }
}

static void explist_nonlinear (LexState *ls, expdesc *v, const std::vector<size_t>& argtis, std::vector<void*>& prop) {
  prop.reserve(argtis.size());
  explist_nonlinear_arg(ls, v, argtis.at(0), *(TypeHint*)prop.emplace_back(new_typehint(ls)));
  for (size_t i = 1; i != argtis.size(); ++i) {
    luaK_exp2nextreg(ls->fs, v);
    explist_nonlinear_arg(ls, v, argtis.at(i), *(TypeHint*)prop.emplace_back(new_typehint(ls)));
  }
}

static int explist (LexState *ls, expdesc *v, TypeHint *prop = nullptr) {
  /* explist -> expr { ',' expr } */
  int n = 1;  /* at least one expression */
  expr(ls, v, prop);
  if (prop)
    exp_propagate(ls, *v, *prop);
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(ls->fs, v);
    expr(ls, v);
    n++;
  }
  return n;
}

static bool isnamedarg (LexState *ls) {
  return ls->t.token != TK_EOS && luaX_lookahead(ls) == '=';
}

static void funcargs (LexState *ls, expdesc *f, TypeDesc *funcdesc = nullptr) {
  FuncState *fs = ls->fs;
  expdesc args;
  FuncArgsState& fas = ls->funcargsstates.emplace();
  int base, nparams;
  const auto line = ls->getLineNumber();
  switch (ls->t.token) {
    case '(': {  /* funcargs -> '(' [ explist ] ')' */
      luaX_next(ls);
      if (ls->t.token == ')')  /* arg list is empty? */
        args.k = VVOID;
      else {
        int num_positional_args = 0;
        if (!isnamedarg(ls)) {
          num_positional_args++;
          expr_propagate(ls, &args, *(TypeHint*)fas.argdescs.emplace_back(new_typehint(ls)));
          while (testnext(ls, ',')) {
            luaK_exp2nextreg(ls->fs, &args);
            if (isnamedarg(ls))
              break;
            expr_propagate(ls, &args, *(TypeHint*)fas.argdescs.emplace_back(new_typehint(ls)));
            num_positional_args++;
          }
        }
        if (ls->t.token != ')') {
          if (!isnamedarg(ls)) {  /* is this not a named argument? */
            error_expected(ls, ')');  /* then raise syntax error similar to Lua */
          }
          if (!funcdesc) {
            luaX_syntaxerror(ls, "can't used named arguments here because the function was not found at parse-time");
          }
          std::vector<size_t> argtis{};
          argtis.resize(funcdesc->getNumParams() - num_positional_args);
          do {
            TString *pname = str_checkname(ls, 0);
            int pi = funcdesc->findParamByName(pname);
            if (pi == -1) {
              throwerr(ls, luaO_fmt(ls->L, "function does not have a %s parameter", getstr(pname)), "unknown parameter");
            }
            if (num_positional_args > pi) {
              throwerr(ls, luaO_fmt(ls->L, "%s parameter was already assigned to positionally", getstr(pname)), "double-assignment of parameter");
            }
            pi -= num_positional_args;
            checknext(ls, '=');
            argtis.at(pi) = luaX_getpos(ls);
            skip_over_simpleexp_within_parenlist(ls);
          } while (testnext(ls, ','));
          const auto tidx = luaX_getpos(ls);
          explist_nonlinear(ls, &args, argtis, fas.argdescs);
          luaX_setpos(ls, tidx);
        }
        if (hasmultret(args.k) && args.k != VSAFECALL)
          luaK_setmultret(fs, &args);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    case '{': {  /* funcargs -> constructor */
      auto hint = new_typehint(ls);
      hint->emplaceTypeDesc(VT_TABLE);
      fas.argdescs = { hint };
      constructor(ls, &args);
      break;
    }
    case TK_STRING: {  /* funcargs -> STRING */
      auto hint = new_typehint(ls);
      hint->emplaceTypeDesc(VT_STR);
      fas.argdescs = { hint };
      codestring(&args, ls->t.seminfo.ts);
      luaX_next(ls);  /* must use 'seminfo' before 'next' */
      break;
    }
    default: {
      luaX_syntaxerror(ls, "function arguments expected");
    }
  }
  if (funcdesc) {
    for (lu_byte i = 0; i != funcdesc->getNumTypedParams(); ++i) {
      const TypeHint* param_hint = funcdesc->params[i];
      if (param_hint->empty())
        continue; /* skip parameters without type hint */
      TypeHint arg{};
      if (i < (int)fas.argdescs.size()) {
        arg = *(TypeHint*)fas.argdescs.at(i);
        if (arg.empty())
          continue; /* skip arguments without propagated type */
      }
      if (!param_hint->isCompatibleWith(arg)) {
        std::string err = "Function's '";;
        err.append(getstr(funcdesc->proto->locvars[i].varname), tsslen(funcdesc->proto->locvars[i].varname));
        err.append("' parameter was type-hinted as ");
        err.append(param_hint->toString());
        err.append(" but provided with ");
        err.append(arg.toString());
        throw_warn(ls, err.c_str(), "argument type mismatch", line, WT_TYPE_MISMATCH);
      }
    }
    const auto expected = funcdesc->getNumParams();
    const auto received = (int)fas.argdescs.size();
    if (!funcdesc->proto->is_vararg && expected < received) {  /* Too many arguments? */
      auto suffix = expected == 1 ? "" : "s"; // Omit plural suffixes when the noun is singular.
      throw_warn(ls,
        "too many arguments",
          luaO_fmt(ls->L, "expected %d argument%s, got %d.", expected, suffix, received), line, WT_EXCESSIVE_ARGUMENTS);
      --ls->L->top.p;
    }
    ls->nodiscard = funcdesc->nodiscard;
  }
  else {
    ls->nodiscard = false;
  }
  lua_assert(f->k == VNONRELOC);
  base = f->u.reg;  /* base register for call */
  if (hasmultret(args.k) && args.k != VSAFECALL)
    nparams = LUA_MULTRET;  /* open call */
  else {
    if (args.k != VVOID)
      luaK_exp2nextreg(fs, &args);  /* close last argument */
    nparams = fs->freereg - (base+1);
  }
  init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams+1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base+1;  /* call remove function and arguments and leaves
                            (unless changed) one result */
  ls->funcargsstates.pop();
}




/*
** {======================================================================
** Expression parsing
** =======================================================================
*/


static void method_call_funcargs (LexState *ls, expdesc *v) {
  if (testnext(ls, '?')) {
    FuncState *fs = ls->fs;
    luaK_codeABC(fs, OP_TEST, v->u.reg, NO_REG, 0);
    int jf = luaK_jump(fs);
    funcargs(ls, v);
    lua_assert(v->k == VCALL);
    const auto pc = v->u.pc;
    luaK_exp2nextreg(fs, v);
    lua_assert(v->k == VNONRELOC);
    v->k = VSAFECALL;
    v->u.pc = pc;  /* instruction pc */
    int jt = luaK_jump(fs);
    luaK_patchtohere(fs, jf);
    luaK_nil(fs, v->u.reg + 1, 1);  /* failed to call, push nil */
    luaK_patchtohere(fs, jt);
  }
  else
    funcargs(ls, v);
}

/*
** Safe navigation is based on a patch published by SvenOlsen.
** http://lua-users.org/wiki/SvenOlsen
*/
static void safe_navigation (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  luaK_exp2nextreg(fs, v);
  luaK_codeABC(fs, OP_TEST, v->u.reg, NO_REG, 0);
  const bool is_method_call = (ls->t.token == ':');
  int jt;
  {
    int old_free = fs->freereg;
    int vreg = v->u.reg;
    int jf = luaK_jump(fs);
    expdesc key;
    switch (ls->t.token) {
      case '[': {
        luaX_next(ls);  /* skip the '[' */
        expr(ls, &key);
        checknext(ls, ']');
        luaK_indexed(fs, v, &key);
        break; 
      }       
      case '.': {
        luaX_next(ls);
        codename(ls, &key);
        luaK_indexed(fs, v, &key);
        break;
      }
      case ':': {
        luaX_next(ls);
        codename(ls, &key, N_RESERVED);
        luaK_self(fs, v, &key);
        method_call_funcargs(ls, v);
        break;
      }
      default: {
        luaX_syntaxerror(ls, "unexpected symbol");
      }
    }
    luaK_exp2nextreg(fs, v);
    fs->freereg = old_free;
    if (v->u.reg != vreg) {
      luaK_codeABC(fs, OP_MOVE, vreg, v->u.reg, 0);
      v->u.reg = vreg;
    }
    if (is_method_call) {
      jt = luaK_jump(fs);
    }
    luaK_patchtohere(fs, jf);
  }
  if (is_method_call) {
    v->k = VSAFECALL;
    luaK_nil(fs, v->u.reg + 1, 1);  /* failed to call, push nil */
    luaK_patchtohere(fs, jt);
  }
}


static void top_to_expdesc (LexState *ls, expdesc *v) {
  lua_State *L = ls->L;
  switch (lua_type(L, -1)) {
    case LUA_TNUMBER: {
      if (lua_isinteger(L, -1)) {
        init_exp(v, VKINT, 0);
        v->u.ival = lua_tointeger(L, -1);
      }
      else {
        init_exp(v, VKFLT, 0);
        v->u.nval = lua_tonumber(L, -1);
      }
      break;
    }
    case LUA_TSTRING: {
      size_t len;
      const char* str = lua_tolstring(L, -1, &len);
      codestring(v, luaS_newlstr(L, str, len));
      break;
    }
    case LUA_TTABLE: {
      lua_pushnil(L);
      newtable(ls, v, [ls](expdesc *key, expdesc *val) {
        if (lua_next(ls->L, -2)) {
          top_to_expdesc(ls, val);
          lua_pop(ls->L, 1);
          top_to_expdesc(ls, key);
          return true;
        }
        return false;
      });
      break;
    }
    default: {
      luaX_syntaxerror(ls, "unexpected return value in constexpr_call");
    }
  }
}

static void constexpr_call (LexState *ls, expdesc *v, lua_CFunction f) {
  auto line = ls->getLineNumber();
  checknext(ls, '(');
  lua_State *L = ls->L;
  lua_pushcfunction(L, f);
  int nargs = 0;
  if (ls->t.token != ')') {
    do {
      ++nargs;
      auto argline = ls->getLineNumber();
      expdesc argexp;
      simpleexp(ls, &argexp, E_NO_METHOD_CALL);
      switch (argexp.k) {
        case VKSTR:
          lua_pushlstring(L, getstr(argexp.u.strval), tsslen(argexp.u.strval));
          break;
        case VKINT:
          lua_pushinteger(L, argexp.u.ival);
          break;
        case VKFLT:
          lua_pushnumber(L, argexp.u.nval);
          break;
        case VTRUE:
          lua_pushboolean(L, true);
          break;
        case VFALSE:
          lua_pushboolean(L, false);
          break;
        case VNIL:
          lua_pushnil(L);
          break;
        case VCONST:
          lua_lock(L);
          *s2v(L->top.p) = ls->dyd->actvar.arr[argexp.u.info].k;
          api_incr_top(L);
          lua_unlock(L);
          break;
        default:
          throwerr(ls, "unexpected argument type in constexpr_call", "unexpected argument type", argline);
      }
    } while (testnext(ls, ','));
  }
  check_match(ls, ')', '(', line);
  int status = lua_pcall(L, nargs, 1, 0);
  if (status != LUA_OK) {
    throwerr(ls, lua_tostring(L, -1), "error in constexpr_call", line);
  }
  top_to_expdesc(ls, v);
  lua_pop(L, 1);
}


static bool check_constexpr_call (LexState *ls, expdesc *v, const char *name, lua_CFunction f) {
  if (strcmp(getstr(ls->t.seminfo.ts), name) == 0) {
    luaX_next(ls); /* skip TK_NAME */
    constexpr_call(ls, v, f);
    return true;
  }
  return false;
}

int luaB_tonumber (lua_State *L);
int luaB_utonumber (lua_State *L);
int luaB_tostring (lua_State *L);
int luaB_utostring (lua_State *L);
int luaB_type (lua_State *L);

static void const_expr (LexState *ls, expdesc *v) {
  switch (ls->t.token) {
    case TK_NAME: {
      const Pluto::ConstexprLibrary* lib = nullptr;
      for (const auto& library : Pluto::all_preloaded) {
        if (strcmp(library->name, getstr(ls->t.seminfo.ts)) == 0) {
          lib = library;
          break;
        }
      }
      if (lib == nullptr) {
        for (const auto& library : Pluto::all_constexpr) {
          if (strcmp(library->name, getstr(ls->t.seminfo.ts)) == 0) {
            lib = library;
            break;
          }
        }
      }
      if (lib != nullptr) {
        luaX_next(ls); /* skip TK_NAME */
        checknext(ls, '.');
        check(ls, TK_NAME);
        lua_CFunction f = NULL;
        for (auto reg = &lib->funcs[0]; reg->name; ++reg) {
          if (strcmp(reg->name, getstr(ls->t.seminfo.ts)) == 0) {
            f = reg->func;
            break;
          }
        }
        if (f == NULL) {
          throwerr(ls, luaO_fmt(ls->L, "%s is not a member of %s", getstr(ls->t.seminfo.ts), lib->name), "unknown function.");
        }
        else {
          luaX_next(ls); /* skip TK_NAME */
          constexpr_call(ls, v, f);
        }
      }
      else if (!check_constexpr_call(ls, v, "tonumber", luaB_tonumber)
          && !check_constexpr_call(ls, v, "utonumber", luaB_utonumber)
          && !check_constexpr_call(ls, v, "tostring", luaB_tostring)
          && !check_constexpr_call(ls, v, "utostring", luaB_utostring)
          && !check_constexpr_call(ls, v, "type", luaB_type)
        )
      {
        throwerr(ls, luaO_fmt(ls->L, "%s is not available in constant expression", getstr(ls->t.seminfo.ts)), "unrecognized name.");
      }
      return;
    }
    default: {
      const char *token = luaX_token2str(ls, ls->t);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void enumexp (LexState *ls, expdesc *v, TString *varname) {
  switch (ls->t.token) {
    case ':': {
      luaX_next(ls);
#ifdef PLUTO_PARSER_SUGGESTIONS
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("efunc", "values");
        ss.push("efunc", "names");
        ss.push("efunc", "kvmap");
        ss.push("efunc", "vkmap");
      }
#endif
      check(ls, TK_NAME);
      if (strcmp(getstr(ls->t.seminfo.ts), "values") == 0) {
        luaX_next(ls);
        checknext(ls, '('); checknext(ls, ')');
        const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
        size_t i = 0;
        newtable(ls, v, [ed, &i](expdesc *e) {
          if (i == ed->enumerators.size())
            return false;
          init_exp(e, VKINT, 0);
          e->u.ival = ed->enumerators.at(i++).value;
          return true;
        });
      }
      else if (strcmp(getstr(ls->t.seminfo.ts), "names") == 0) {
        luaX_next(ls);
        checknext(ls, '('); checknext(ls, ')');
        const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
        size_t i = 0;
        newtable(ls, v, [ed, &i](expdesc *e) {
          if (i == ed->enumerators.size())
            return false;
          init_exp(e, VKSTR, 0);
          e->u.strval = ed->enumerators.at(i++).name;
          return true;
        });
      }
      else if (strcmp(getstr(ls->t.seminfo.ts), "kvmap") == 0) {
        luaX_next(ls);
        checknext(ls, '('); checknext(ls, ')');
        const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
        size_t i = 0;
        newtable(ls, v, [ed, &i](expdesc *key, expdesc *val) {
          if (i == ed->enumerators.size())
            return false;
          init_exp(key, VKSTR, 0);
          key->u.strval = ed->enumerators.at(i).name;
          init_exp(val, VKINT, 0);
          val->u.ival = ed->enumerators.at(i).value;
          i++;
          return true;
        });
      }
      else if (strcmp(getstr(ls->t.seminfo.ts), "vkmap") == 0) {
        luaX_next(ls);
        checknext(ls, '('); checknext(ls, ')');
        const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
        size_t i = 0;
        newtable(ls, v, [ed, &i](expdesc *key, expdesc *val) {
          if (i == ed->enumerators.size())
            return false;
          init_exp(key, VKINT, 0);
          key->u.ival = ed->enumerators.at(i).value;
          init_exp(val, VKSTR, 0);
          val->u.strval = ed->enumerators.at(i).name;
          i++;
          return true;
        });
      }
      else {
        throwerr(ls, luaO_fmt(ls->L, "%s is not a member of enums", getstr(ls->t.seminfo.ts)), "unknown member.");
      }
      return;
    }
    case '.': {
      const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
      luaX_next(ls);
#ifdef PLUTO_PARSER_SUGGESTIONS
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        for (const auto& e : ed->enumerators) {
          ss.push("eprop", getstr(e.name), std::to_string(e.value));
        }
      }
#endif
      check(ls, TK_NAME);
      for (const auto& e : ed->enumerators) {
        if (eqstr(e.name, ls->t.seminfo.ts)) {
          init_exp(v, VKINT, 0);
          v->u.ival = e.value;
          luaX_next(ls);
          return;
        }
      }
      throwerr(ls, luaO_fmt(ls->L, "%s is not a member of %s", getstr(ls->t.seminfo.ts), getstr(varname)), "unknown member.");
      return;
    }
    default: {
      const char *token = luaX_token2str(ls, ls->t);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void selfexp (LexState *ls, expdesc *v) {
  bool ismethod = testnext(ls, ':');
  if (testnext(ls, '.') || ismethod) {
    if (!ismethod) {
      luaK_exp2anyregup(ls->fs, v);
    }
    expdesc key;
    TString *keystr = str_checkname(ls, N_RESERVED);

    if (auto special = ls->classes.top().getSpecialName(keystr); special.has_value()) {
      codestring(&key, luaX_newstring(ls, special.value().c_str()));
    }
    else {
      codestring(&key, keystr);
    }

    if (!ismethod) {
      luaK_indexed(ls->fs, v, &key);
    }
    else {
      luaK_self(ls->fs, v, &key);
      method_call_funcargs(ls, v);
    }
  }
}


static void parentexp (LexState *ls, expdesc *v) {
  if (const auto parent_pos = ls->getParentClassPos()) {
    if (testnext(ls, ':')) {
      auto pos = luaX_getpos(ls);
      luaX_setpos(ls, parent_pos);
      classname(ls, v);
      luaX_setpos(ls, pos);
      luaK_exp2nextreg(ls->fs, v);

      expdesc key;
      codename(ls, &key);
      luaK_indexed(ls->fs, v, &key);
      luaK_exp2nextreg(ls->fs, v);

      expdesc first_arg;
      singlevar(ls, &first_arg, luaS_newliteral(ls->L, "self"));
      luaK_exp2nextreg(ls->fs, &first_arg);

      funcargs(ls, v);
    }
    else {
      singlevar(ls, v, luaS_newliteral(ls->L, "self"));
      luaK_exp2anyregup(ls->fs, v);
      expdesc key;
      codestring(&key, luaS_newliteral(ls->L, "__parent"));
      luaK_indexed(ls->fs, v, &key);
    }
  }
  else {
    throw_warn(ls, "'parent' used outside of a child class, defering to global called 'parent'", WT_BAD_PRACTICE);
    singlevar(ls, v, luaS_newliteral(ls->L, "parent"), false);  /* defer to global 'parent' */
  }
}


static void expsuffix (LexState* ls, expdesc* v, int line, int flags, TypeHint *prop);

static void primaryexp (LexState *ls, expdesc *v, int flags = 0) {
  /* primaryexp -> NAME | '(' expr ')' */
  if (isnametkn(ls, N_OVERRIDABLE)) {
    const bool is_overridable = ls->t.IsOverridable();
    TString *varname = str_checkname(ls, N_OVERRIDABLE);
    if (gett(ls) == TK_WALRUS) {
      if (flags & E_OR_KILLED_WALRUS)
        throwerr(ls, "':=' is not allowed in this context", "due to the 'or', it is no longer guaranteed that the local will be initialized by the time it's in scope.");
      if (!(flags & E_WALRUS) || ls->fs->freereg != luaY_nvarstack(ls->fs))
        throwerr(ls, "':=' is only allowed in 'if' and 'while' statements", "unexpected ':='");
      luaX_next(ls);
      new_localvar(ls, varname);
      expr(ls, v);
      adjust_assign(ls, 1, 1, v);
      adjustlocalvars(ls, 1);
      ls->used_walrus = true;
    }
    else
      singlevar(ls, v, varname, is_overridable);
    if (!ls->classes.empty() && strcmp(getstr(varname), "self") == 0) {
      selfexp(ls, v);
      return;
    }
    if (v->k == VENUM) {
      enumexp(ls, v, varname);
    }
    if (is_overridable && v->k == VVOID)
      luaX_prev(ls);
    else
      return;
  }
  switch (ls->t.token) {
    case '(': {
      int line = ls->getLineNumber();
      luaX_next(ls);
      expr(ls, v, nullptr, flags & (E_WALRUS | E_OR_KILLED_WALRUS));
      check_match(ls, ')', '(', line);
      luaK_dischargevars(ls->fs, v);
      return;
    }
    case '$': {
      luaX_next(ls); /* skip '$' */
      const_expr(ls, v);
      return;
    }
    case TK_PARENT:
    case TK_PPARENT: {
      luaX_next(ls);
      parentexp(ls, v);
      return;
    }
    case TK_STRING: {
      codestring(v, ls->t.seminfo.ts);
      luaX_next(ls);
      expsuffix(ls, v, ls->getLineNumber(), flags, nullptr);
      return;
    }
    case '{': {
      throwerr(ls, "unexpected symbol near '{'", "if you meant to begin this statement with a table, wrap it in parentheses.");
    }
    default: {
      if (ls->t.token == ')' && ls->getContext() == PARCTX_BODY) {
        throwerr(ls, "unexpected ')', expected 'end' to close function.", "missing 'end' before ')'.");
      }
      const char *token = luaX_token2str(ls, ls->t);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void suffixedexp (LexState *ls, expdesc *v, int flags = 0, TypeHint *prop = nullptr) {
  /* suffixedexp ->
       primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
  int line = ls->getLineNumber();
  primaryexp(ls, v, flags);
  if (prop) {
    if (v->k == VINDEXUP) {
      TValue *key = &ls->fs->f->k[v->u.ind.idx];
      prop->merge(get_global_prop(ls, tsvalue(key)));
    }
  }
  expsuffix(ls, v, line, flags, prop);
}

static void expsuffix (LexState *ls, expdesc *v, int line, int flags, TypeHint *prop) {
  FuncState *fs = ls->fs;
  for (;;) {
    switch (ls->t.token) {
      case '?': {  /* safe navigation or ternary */
        auto t = luaX_lookahead(ls);
        if (t != '[' && t != '.' && t != ':') {
          /* it's a ternary but we have to deal with that later */
          return; /* back to primaryexp */
        }
        luaX_next(ls); /* skip '?' */
        safe_navigation(ls, v);
        break;
      }
      case '.': {  /* fieldsel */
        fieldsel(ls, v);
        break;
      }
      case '[': {  /* '[' exp ']' */
        expdesc key;
        luaK_exp2anyregup(fs, v);
        yindex(ls, &key);
        luaK_indexed(fs, v, &key);
        break;
      }
      case ':': {  /* ':' NAME funcargs */
        if ((flags & E_NO_METHOD_CALL) || (flags & E_NO_CONSUME_COLON)) {
          return;
        }
        expdesc key;
        const auto colon_line = ls->t.line;
        const auto colon_column = ls->t.column;
        luaX_next(ls);  /* skip ':' */
        if (l_unlikely(colon_line != ls->t.line)) {
          throw_warn(ls, "possibly unwanted function call", luaO_fmt(ls->L, "possibly unwanted continuation of the expression on line %d.", colon_line), WT_POSSIBLE_TYPO);
          ls->L->top.p--;
        }
        else if (l_unlikely(ls->t.column != (colon_column + 1) && ls->getContext() == PARCTX_TERNARY_C)) {
          throw_warn(ls, "possible confusion with colons", "the second colon is interpreted as a method call instead of the first colon", "wrap the method call in parentheses", ls->t.line, WT_POSSIBLE_TYPO);
        }
        codename(ls, &key, N_RESERVED);
        luaK_self(fs, v, &key);
        method_call_funcargs(ls, v);
        break;
      }
      case '(': case TK_STRING: case '{': {  /* funcargs */
        if (flags & E_NO_CALL) {
          return;
        }
        if ((flags & E_PIPERHS) && ls->t.token == '(') {
          if (luaX_lookbehind(ls).line == ls->t.line)
            throw_warn(ls, "possible syntax confusion", "'(' is ignored by the pipe operator. use '|' if you meant to pass additional arguments.", WT_POSSIBLE_TYPO);
          return;
        }
        if (luaX_lookbehind(ls).line != ls->t.line && (ls->getContext() == PARCTX_LAMBDA_BODY || v->k == VCALL)) {
          throw_warn(ls, "possibly unwanted function call", luaO_fmt(ls->L, "possibly unwanted continuation of the expression on line %d.", luaX_lookbehind(ls).line), WT_POSSIBLE_TYPO);
          ls->L->top.p--;
        }
        Vardesc *vd;
        TypeDesc *funcdesc = nullptr;
        if (v->k == VLOCAL) {
          vd = getlocalvardesc(ls->fs, v->u.var.vidx);
        _funcdesc_from_vd:
          if (vd->vd.prop->descs[0].type == VT_FUNC  /* just in case... */
            && vd->vd.prop->descs[0].proto != nullptr  /* real function/not just a hint? */
          ) {
            funcdesc = &vd->vd.prop->descs[0];
            if (prop) { /* propagate return type */
              *prop = *vd->vd.prop->descs[0].retn;
            }
          }
        }
        else if (v->k == VUPVAL) {
          Upvaldesc *uv = &ls->fs->f->upvalues[v->u.info];
          FuncState *efs = ls->fs->prev;
          int idx = uv->idx;
          while (efs) { /* efs == nullptr && instack if resolving _ENV */
            if (uv->instack) {
              for (int i = 0; i != efs->nactvar; ++i) {
                vd = getlocalvardesc(efs, i);
                if (vd->vd.kind != RDKCTC && vd->vd.kind != RDKENUM  /* is in a register? */
                  && vd->vd.ridx == idx) {
                  goto _funcdesc_from_vd;
                }
              }
              break;
            }
            lua_assert(idx < efs->nups);
            uv = &efs->f->upvalues[idx];
            efs = efs->prev;
            idx = uv->idx;
          }
        }
        luaK_exp2nextreg(fs, v);
        funcargs(ls, v, funcdesc);
        break;
      }
      case TK_PIPE: {  /* '|>' NAME */
        if ((flags & E_NO_CALL) || (flags & E_PIPERHS)) {
          return;
        }
        luaX_next(ls);
        int nparams = 1;
        if (!(flags & E_NO_METHOD_CALL) && !(flags & E_NO_CONSUME_COLON) && ls->t.token != TK_EOS && luaX_lookahead(ls) == ':') {
          luaK_reserveregs(fs, 2);
          luaK_exp2nextreg(fs, v);
          fs->freereg -= 3;
          primaryexp(ls, v);
          checknext(ls, ':');
          expdesc key;
          codename(ls, &key);
          luaK_self(fs, v, &key);
          ++nparams;
        }
        else {
          luaK_exp2nextreg(fs, v);
          expdesc func;
          simpleexp(ls, &func, flags | E_PIPERHS);
          luaK_prepcallfirstarg(fs, v, &func);
        }
        lua_assert(v->k == VNONRELOC);
        int base = v->u.reg;  /* base register for call */
        if (testnext(ls, '|')) {
          do {
            expdesc arg;
            expr(ls, &arg, nullptr, E_NO_BOR);
            luaK_exp2nextreg(fs, &arg);
            ++nparams;
          } while (testnext(ls, ','));
          checknext(ls, '|');
        }
        init_exp(v, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
        luaK_fixline(fs, line);
        fs->freereg = base + 1;
        if (ls->t.token == '(' || ls->t.token == TK_STRING || ls->t.token == '{')
          return;
        break;
      }
      default: return;
    }
  }
}


static int cond (LexState *ls, bool for_while_loop = false);
static void ifexpr (LexState *ls, expdesc *v) {
  /*
  ** 'if' expressions are based on a patch published by by Ryota Hirose.
  */
  FuncState *fs = ls->fs;
  int condition;
  int escape = NO_JUMP;
  int reg;
  luaX_next(ls);
  condition = cond(ls);
  checknext(ls, TK_THEN);
  expr(ls, v);
  reg = luaK_exp2anyreg(fs, v);
  luaK_concat(fs, &escape, luaK_jump(fs));
  luaK_patchtohere(fs, condition);
  checknext(ls, TK_ELSE);
  expr(ls, v);
  checknext(ls, TK_END);
  luaK_exp2reg(fs, v, reg);
  luaK_patchtohere(fs, escape);
}


static void newexpr (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;

  luaX_next(ls);

  singlevaraux(fs, luaS_newliteral(ls->L, "Pluto_operator_new"), v, 1);
  lua_assert(v->k != VVOID);
  luaK_exp2nextreg(fs, v);

  expdesc first_arg;
  expr(ls, &first_arg, nullptr, E_NO_CALL);
  luaK_exp2nextreg(fs, &first_arg);

  funcargs(ls, v);
}


static BinOpr subexpr (LexState *ls, expdesc *v, int limit, TypeHint *prop = nullptr, int flags = 0);


static BinOpr custombinaryoperator (LexState *ls, expdesc *v, int flags, TString *impl) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();

  expdesc func;
  singlevaraux(fs, impl, &func, 1);
  lua_assert(v->k != VVOID);
  luaK_prepcallfirstarg(fs, v, &func);
  lua_assert(v->k == VNONRELOC);
  int base = v->u.reg;  /* base register for call */

  expdesc arg2;
  auto nextop = subexpr(ls, &arg2, 3, nullptr, flags);
  luaK_exp2nextreg(fs, &arg2);

  int nparams = fs->freereg - (base + 1);
  init_exp(v, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base + 1;

  return nextop;
}


static void lgoto (LexState *ls, TString *name, int line) {
  FuncState *fs = ls->fs;
  Labeldesc *lb = findlabel(ls, name);
  if (lb == NULL)  /* no label? */
    /* forward jump; will be resolved when the label is declared */
    newgotoentry(ls, name, line, luaK_jump(fs), false);
  else {  /* found a label */
    /* backward jump; will be resolved here */
    int lblevel = reglevel(fs, lb->nactvar);  /* label level */
    if (luaY_nvarstack(fs) > lblevel)  /* leaving the scope of a variable? */
      luaK_codeABC(fs, OP_CLOSE, lblevel, 0, 0);
    /* create jump and link it to the label */
    luaK_patchlist(fs, luaK_jump(fs), lb->pc);
  }
}


static std::vector<int> casecond (LexState *ls, const expdesc& ctrl, int tk) {
  std::vector<int> jumps{};
  FuncState *fs = ls->fs;
  const auto case_line = ls->getLineNumber();

  int expr_flags = 0;
  if (tk == ':') {
    expr_flags |= E_NO_CONSUME_COLON;
  }

  expdesc e, cmpval;
  e = ctrl;
  luaK_infix(fs, OPR_EQ, &e);
  expr(ls, &cmpval, nullptr, expr_flags);
  luaK_posfix(fs, OPR_EQ, &e, &cmpval, case_line);
  jumps.emplace_back(e.u.pc);
  while (testnext(ls, ',')) {
    e = ctrl;
    luaK_infix(fs, OPR_EQ, &e);
    expr(ls, &cmpval, nullptr, expr_flags);
    luaK_posfix(fs, OPR_EQ, &e, &cmpval, case_line);
    jumps.emplace_back(e.u.pc);
  }
  checknext(ls, tk);

  return jumps;
}

static void switchimpl (LexState *ls, int tk, void(*caselist)(LexState*,void*), void *ud = nullptr) {
  const auto line = ls->getLineNumber();
  const auto switchToken = gett(ls);
  luaX_next(ls); // Skip switch statement.

  ls->switchstates.emplace();
  FuncState *fs = ls->fs;
  BlockCnt sbl;

  lu_byte freereg = fs->freereg;
  if (tk == TK_ARROW)
    fs->freereg = luaY_nvarstack(fs); // To prevent assert in enterblock
  enterblock(fs, &sbl, BlockType::BT_BREAK);
  fs->freereg = freereg;

  int prevpinnedreg = -1;
  expdesc ctrl;
  expr(ls, &ctrl);
  checknext(ls, TK_DO);
  if (!vkhasregister(ctrl.k)
    || ctrl.t != ctrl.f  /* has jumps? */
  ) {
    luaK_exp2nextreg(ls->fs, &ctrl);
    if (tk == TK_ARROW) {
      prevpinnedreg = fs->pinnedreg;
      fs->pinnedreg = ctrl.u.reg;
    }
    else {
      new_localvarliteral(ls, "(switch control value)"); // Save control value into a local.
      adjustlocalvars(ls, 1);
    }
  }

  const auto nactvar = fs->nactvar;

  std::vector<int>& first = ls->switchstates.top().first;
  int default_pc = -1;
  int first_pc, goto_begin_pc;

  if (gett(ls) == TK_CASE) {
    luaX_next(ls); /* Skip 'case' */
    first = casecond(ls, ctrl, tk);
    first_pc = luaK_getlabel(fs);
    caselist(ls, ud);
  }
  else {
    goto_begin_pc = luaK_jump(fs);
  }

  std::vector<SwitchCase>& cases = ls->switchstates.top().cases;

  while (gett(ls) != TK_END) {
    auto case_line = ls->getLineNumber();
    if (fs->nactvar != nactvar) {
      Vardesc *var = getlocalvardesc(ls->fs, nactvar);
      const char *msg = "this case jumps into the scope of local '%s' defined on line %d";
      msg = luaO_pushfstring(ls->L, msg, getstr(var->vd.name), var->vd.line);
      luaK_semerror(ls, msg);  /* raise the error */
    }
    if (gett(ls) == TK_DEFAULT) {
      luaX_next(ls); /* Skip 'default' */
      checknext(ls, tk);
      if (default_pc != -1)
        throwerr(ls, "switch statement already has a default case", "second default case", case_line);
      default_pc = luaK_getlabel(fs);
    }
    else {
      checknext(ls, TK_CASE);
      cases.emplace_back(SwitchCase{ luaX_getpos(ls), luaK_getlabel(fs) });
      skip_until(ls, tk); /* skip over casecond */
      checknext(ls, tk);
    }
    ls->laststat.token = TK_EOS;  /* We don't want warnings for trailing control flow statements. */
    caselist(ls, ud);
  }

  if (ls->laststat.token != TK_BREAK) {  /* last block did not have 'break'? */
    if (tk == ':') {  /* switch statement? */
      /* jump to the end of switch as otherwise we would loop infinitely */
      lbreak(ls, 1, ls->getLineNumber(), luaK_jump(fs));
    }
  }

  /* if switch expression has no default case, generate one to guarantee nil in that case
     otherwise, the value returned by the expression would be whatever was in the register before */
  if (tk == TK_ARROW && default_pc == -1) {
    default_pc = luaK_getlabel(fs);
    const auto line = ls->getLineNumber();
    const auto reg = reinterpret_cast<expdesc*>(ud)->u.reg;
    expdesc cv;
    init_exp(&cv, VNIL, 0);
    luaK_exp2reg(ls->fs, &cv, reg);
    lbreak(ls, 1, line, luaK_jump(fs));
  }

  if (!first.empty()) {
    for (int i = 0; i != first.size() - 1; ++i) {
      luaK_patchlist(fs, first.at(i), first_pc);
    }
    luaK_invertcond(fs, first.back());
    luaK_patchtohere(fs, first.back());
  }
  else {
    luaK_patchtohere(fs, goto_begin_pc);
  }

  /* prune cases that lead to default case */
  if (default_pc != -1) {
    for (auto i = cases.begin(); i != cases.end(); ) {
      if (i->pc == default_pc) {
        i = cases.erase(i);
      }
      else {
        ++i;
      }
    }
  }

  int nactvarend = ls->fs->nactvar;
  ls->fs->nactvar = nactvar;  /* variables declared inside of switch body don't exist yet */
  for (auto& c : cases) {
    auto pos = luaX_getpos(ls);
    luaX_setpos(ls, c.tidx);
    for (const auto& j : casecond(ls, ctrl, tk)) {
      luaK_patchlist(fs, j, c.pc);
    }
    luaX_setpos(ls, pos);
  }
  ls->fs->nactvar = nactvarend;

  if (default_pc != -1)
    luaK_jumpto(fs, default_pc);

  if (tk == TK_ARROW && fs->pinnedreg != -1) {
    fs->pinnedreg = prevpinnedreg;
    luaK_freeexp(fs, &ctrl);
  }

  check_match(ls, TK_END, switchToken, line);
  leaveblock(fs);
  if (tk == TK_ARROW)
    fs->freereg = freereg;
  ls->switchstates.pop();
}

static void switchstat (LexState *ls) {
  switchimpl(ls, ':', [](LexState* ls, void*) {
    const int case_line = luaX_lookbehind(ls).line;
    if (gett(ls) == TK_CASE || gett(ls) == TK_DEFAULT || gett(ls) == TK_END) {
      /* this case is empty, nothing to do */
    }
    else if (gett(ls) == TK_FALLTHROUGH) {
      /* empty case explicitly declaring itself to be a fallthrough */
      luaX_next(ls);
    }
    else {
      do {
        statement(ls);
      }
      while (gett(ls) != TK_CASE && gett(ls) != TK_DEFAULT && gett(ls) != TK_END && gett(ls) != TK_FALLTHROUGH);
      if (ls->t.token == TK_CASE && ls->laststat.token != TK_BREAK && ls->laststat.token != TK_RETURN && ls->laststat.token != TK_GOTO) {
        throw_warn(ls, "possibly unwanted fallthrough", luaO_fmt(ls->L, "the case on line %d flows into this case", case_line), "place `--@fallthrough` before this case if this is intended", ls->getLineNumber(), WT_UNANNOTATED_FALLTHROUGH);
        ls->L->top.p--;
      }
      else testnext(ls, TK_FALLTHROUGH);
    }
  });
}

static void switchexpr (LexState *ls, expdesc *v) {
  init_exp(v, VNONRELOC, ls->fs->freereg);
  luaK_reserveregs(ls->fs, 1);
  switchimpl(ls, TK_ARROW, [](LexState *ls, void *ud) {
    const auto line = ls->getLineNumber();
    const auto reg = reinterpret_cast<expdesc*>(ud)->u.reg;
    expdesc cv;
    expr(ls, &cv);
    luaK_exp2reg(ls->fs, &cv, reg);
    lbreak(ls, 1, line, luaK_jump(ls->fs));
  }, v);
}


static void simpleexp (LexState *ls, expdesc *v, int flags, TypeHint *prop) {
  /* simpleexp -> FLT | INT | STRING | NIL | TRUE | FALSE | ... |
                  constructor | FUNCTION body | suffixedexp */
  check_for_non_portable_code(ls);
  switch (ls->t.token) {
    case TK_FLT: {
      if (prop) prop->emplaceTypeDesc(VT_FLT);
      init_exp(v, VKFLT, 0);
      v->u.nval = ls->t.seminfo.r;
      luaX_next(ls);
      return;
    }
    case TK_INT: {
      if (prop) prop->emplaceTypeDesc(VT_INT);
      init_exp(v, VKINT, 0);
      v->u.ival = ls->t.seminfo.i;
      luaX_next(ls);
      return;
    }
    case TK_STRING: {
      if (prop) prop->emplaceTypeDesc(VT_STR);
      codestring(v, ls->t.seminfo.ts);
      luaX_next(ls);
      if (ls->t.token == '[' || ls->t.token == ':' || ls->t.token == TK_PIPE)
        break;
      return;
    }
    case TK_NIL: {
      if (prop) prop->emplaceTypeDesc(VT_NIL);
      init_exp(v, VNIL, 0);
      luaX_next(ls);
      return;
    }
    case TK_TRUE: {
      if (prop) prop->emplaceTypeDesc(VT_BOOL);
      init_exp(v, VTRUE, 0);
      luaX_next(ls);
      return;
    }
    case TK_FALSE: {
      if (prop) prop->emplaceTypeDesc(VT_BOOL);
      init_exp(v, VFALSE, 0);
      luaX_next(ls);
      return;
    }
    case TK_DOTS: {  /* vararg */
      FuncState *fs = ls->fs;
      check_condition(ls, fs->f->is_vararg,
                      "cannot use '...' outside a vararg function");
      init_exp(v, VVARARG, luaK_codeABC(fs, OP_VARARG, 0, 0, 1));
      luaX_next(ls);
      break;
    }
    case '{': {  /* constructor */
      if (prop) prop->emplaceTypeDesc(VT_TABLE);
      constructor(ls, v);
      if (ls->t.token == '[' || ls->t.token == ':' || ls->t.token == '.' || ls->t.token == TK_PIPE)
        break;
      return;
    }
    case TK_FUNCTION: {
      check_condition(ls, !(flags & E_NO_CONSUME_COLON), "cannot instantiate a function in this context");
      luaX_next(ls);
      if (prop) {
        TypeDesc funcdesc;
        body(ls, v, 0, ls->getLineNumber(), &funcdesc);
        prop->emplaceTypeDesc(std::move(funcdesc));
      }
      else {
        body(ls, v, 0, ls->getLineNumber());
      }
      return;
    }
    case '|': {
      check_condition(ls, !(flags & E_NO_CONSUME_COLON), "cannot instantiate a function in this context");
      luaX_next(ls);
      if (prop) {
        TypeDesc funcdesc;
        lambdabody(ls, v, ls->getLineNumber(), &funcdesc);
        prop->emplaceTypeDesc(std::move(funcdesc));
      }
      else {
        lambdabody(ls, v, ls->getLineNumber());
      }
      return;
    }
    case TK_NEW:
    case TK_PNEW: {
      if (prop) prop->emplaceTypeDesc(VT_TABLE);
      newexpr(ls, v);
      break;
    }
    case TK_CLASS:
    case TK_PCLASS: {
      if (prop) prop->emplaceTypeDesc(VT_TABLE);
      luaX_next(ls); /* skip 'class' */
      ls->classes.emplace();
      classexpr(ls, v);
      ls->classes.pop();
      return;
    }
    case TK_SWITCH:
    case TK_PSWITCH: {
      switchexpr(ls, v);
      return;
    }
    default: {
      suffixedexp(ls, v, flags, prop);
      return;
    }
  }
  expsuffix(ls, v, ls->getLineNumber(), flags, prop);
}


static void inexpr (LexState *ls, expdesc *v, int flags) {
  expdesc v2;
  checknext(ls, TK_IN);
  luaK_exp2nextreg(ls->fs, v);
  lua_assert(v->k == VNONRELOC);
  int base = v->u.reg;
  simpleexp(ls, &v2, flags);
  luaK_dischargevars(ls->fs, &v2);
  luaK_exp2nextreg(ls->fs, &v2);
  luaK_codeABC(ls->fs, OP_IN, v->u.reg, v2.u.reg, 0);
  ls->fs->f->onPlutoOpUsed(0);
  ls->fs->freereg = base + 1;
}


static UnOpr getunopr (int op) {
  switch (op) {
    case TK_NOT: return OPR_NOT;
    case '-': return OPR_MINUS;
    case '~': return OPR_BNOT;
    case '#': return OPR_LEN;
    default: return OPR_NOUNOPR;
  }
}


static BinOpr getbinopr (int op) {
  switch (op) {
    case '+': return OPR_ADD;
    case '-': return OPR_SUB;
    case '*': return OPR_MUL;
    case '%': return OPR_MOD;
    case '^': return OPR_POW;
    case '/': return OPR_DIV;
    case TK_IDIV: return OPR_IDIV;
    case '&': return OPR_BAND;
    case '|': return OPR_BOR;
    case '~': return OPR_BXOR;
    case TK_SHL: return OPR_SHL;
    case TK_SHR: return OPR_SHR;
    case TK_CONCAT: return OPR_CONCAT;
    case TK_NE: case TK_NE2: return OPR_NE;
    case TK_EQ: return OPR_EQ;
    case '<': return OPR_LT;
    case TK_LE: return OPR_LE;
    case '>': return OPR_GT;
    case TK_GE: return OPR_GE;
    case TK_SPACESHIP: return OPR_SPACESHIP;
    case TK_INSTANCEOF: return OPR_INSTANCEOF;
    case TK_AND: return OPR_AND;
    case TK_OR: return OPR_OR;
    case TK_COAL: return OPR_COAL;
    case TK_POW: return OPR_POW;  /* '**' operator support */
    default: return OPR_NOBINOPR;
  }
}


static void prefixplusplus (LexState *ls, expdesc *v, bool as_statement) {
  int line = ls->getLineNumber();
  luaX_next(ls); /* skip second '+' */
  if (as_statement)
    suffixedexp(ls, v);
  else
    singlevar(ls, v); /* variable name */
  FuncState *fs = ls->fs;
  expdesc e = *v, v2;
  if (v->k != VLOCAL) {  /* complex lvalue, use a temporary register. linear perf incr. with complexity of lvalue */
    const auto regs_to_reserve = fs->freereg-fs->nactvar;
    luaK_dischargevars(fs, &e);
    luaK_reserveregs(fs, regs_to_reserve);
    enterlevel(ls);
    luaK_infix(fs, OPR_ADD, &e);
    init_exp(&v2, VKINT, 0);
    v2.u.ival = 1;
    luaK_posfix(fs, OPR_ADD, &e, &v2, line);
    leavelevel(ls);
    luaK_exp2nextreg(fs, &e);
    luaK_setoneret(ls->fs, &e);
    luaK_storevar(ls->fs, v, &e);
  }
  else {  /* simple lvalue; a local. directly change value (~20% speedup vs temporary register) */
    enterlevel(ls);
    luaK_infix(fs, OPR_ADD, &e);
    init_exp(&v2, VKINT, 0);
    v2.u.ival = 1;
    luaK_posfix(fs, OPR_ADD, &e, &v2, line);
    leavelevel(ls);
    luaK_setoneret(ls->fs, &e);
    luaK_storevar(ls->fs, v, &e);
  }
}


/*
** Priority table for binary operators.
*/
static const struct {
  lu_byte left;  /* left priority for each binary operator */
  lu_byte right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {10, 10}, {10, 10},           /* '+' '-' */
   {11, 11}, {11, 11},           /* '*' '%' */
   {14, 13},                  /* '^' (right associative) */
   {11, 11}, {11, 11},           /* '/' '//' */
   {6, 6}, {4, 4}, {5, 5},   /* '&' '|' '~' */
   {7, 7}, {7, 7},           /* '<<' '>>' */
   {9, 8},                   /* '..' (right associative) */
   {3, 3}, {3, 3}, {3, 3},   /* ==, <, <= */
   {3, 3}, {3, 3}, {3, 3},   /* ~=, >, >= */
   {3, 3}, {3, 3},           /* <=>, instanceof */
   {2, 2}, {1, 1}, {1, 1},   /* and, or, ?? */
};

#define UNARY_PRIORITY	12  /* priority for unary operators */


[[nodiscard]] static constexpr bool canchainopr (BinOpr opr) noexcept {
  return opr == OPR_LT || opr == OPR_LE || opr == OPR_GT || opr == OPR_GE;
}


/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where 'binop' is any binary operator with a priority higher than 'limit'
*/
static BinOpr subexpr (LexState *ls, expdesc *v, int limit, TypeHint *prop, int flags) {
  BinOpr op;
  UnOpr uop;
  enterlevel(ls);
  uop = getunopr(ls->t.token);
  if (uop != OPR_NOUNOPR) {  /* prefix (unary) operator? */
    int line = ls->getLineNumber();
    luaX_next(ls);  /* skip operator */
    subexpr(ls, v, UNARY_PRIORITY, nullptr, flags);
    luaK_prefix(ls->fs, uop, v, line);
  }
  else if (ls->t.token == TK_IF) ifexpr(ls, v);
  else if (ls->t.token == '+') {
    int line = ls->getLineNumber();
    luaX_next(ls); /* skip '+' */
    if (ls->t.token == '+') { /* '++' ? */
      prefixplusplus(ls, v, false);
    }
    else {
      /* support pseudo-unary '+' by implying '0 + subexpr' */
      init_exp(v, VKINT, 0);
      v->u.ival = 0;
      luaK_infix(ls->fs, OPR_ADD, v);

      expdesc v2;
      subexpr(ls, &v2, priority[OPR_ADD].right, nullptr, flags);
      luaK_posfix(ls->fs, OPR_ADD, v, &v2, line);
    }
  }
  else {
    simpleexp(ls, v, flags, prop);
    if (ls->t.token == TK_IN) {
      throw_warn(ls, "non-portable operator usage", "this operator generates bytecode that is incompatible with Lua.", WT_NON_PORTABLE_BYTECODE);
      inexpr(ls, v, flags);
      if (prop) prop->emplaceTypeDesc(VT_BOOL);
    }
  }
  if (ls->t.token == '+' && luaX_lookahead(ls) == '+') { /* disambiguate '+' operator and '++' as next statement */
    leavelevel(ls);
    return OPR_NOBINOPR;
  }
  /* expand while operators have priorities higher than 'limit' */
  if (l_unlikely(ls->t.token == TK_POW))
    throw_warn(ls, "'**' is deprecated", "use '^' instead", WT_DEPRECATED);
  op = getbinopr(ls->t.token);
  if ((flags & E_NO_BOR) && op == OPR_BOR)
    op = OPR_NOBINOPR;
  while (op != OPR_NOBINOPR && priority[op].left > limit) {
    if (prop && op != OPR_COAL)
      prop->clear();
    expdesc v2;
    BinOpr nextop;
    int line = ls->getLineNumber();
    luaX_next(ls);  /* skip operator */
    if (op == OPR_INSTANCEOF) {
      custombinaryoperator(ls, v, flags, luaS_newliteral(ls->L, "Pluto_operator_instanceof"));
      nextop = getbinopr(ls->t.token);
    }
    else if (op == OPR_SPACESHIP) {
      custombinaryoperator(ls, v, flags, luaS_newliteral(ls->L, "Pluto_operator_spaceship"));
      nextop = getbinopr(ls->t.token);
    }
    else {
      TypeHint *subexpr_prop = nullptr;
      if (op == OPR_CONCAT) {
        if (prop)
          prop->emplaceTypeDesc(VT_STR);
        if (v->t == NO_JUMP && v->f == NO_JUMP) {
          TString *lhs = nullptr;
          if (v->k == VKSTR)
            lhs = v->u.strval;
          else if (v->k == VCONST && ttisstring(&ls->dyd->actvar.arr[v->u.info].k))
            lhs = tsvalue(&ls->dyd->actvar.arr[v->u.info].k);
          if (lhs && ls->t.token == TK_STRING) {
            const auto lhs_size = tsslen(lhs);
            const auto rhs_size = tsslen(ls->t.seminfo.ts);
            auto data = new char[lhs_size + rhs_size];
            memcpy(data, getstr(lhs), lhs_size);
            memcpy(data + lhs_size, getstr(ls->t.seminfo.ts), rhs_size);
            v->k = VKSTR;
            v->u.strval = luaX_newstring(ls, data, lhs_size + rhs_size);
            delete[] data;
            luaX_next(ls);
            op = getbinopr(ls->t.token);
            continue;
          }
        }
      }
      else if (op == OPR_COAL) {
        if (luaK_isalwaysnil(ls, v)) {
          /* weird, but nothing worth talking about... */
        }
        else {
          if (luaK_isalwaystrue(ls, v) || luaK_isalwaysfalse(ls, v))
            throw_warn(ls, "unreachable code", "the expression before the '?\?' is never nil, hence the expression after the '?\?' is never used.", WT_UNREACHABLE_CODE);
        }
        if (prop) {
          prop->erase(VT_NIL);
          subexpr_prop = prop;
        }
      }
      luaK_infix(ls->fs, op, v);
      /* read sub-expression with higher priority */
      if (op == OPR_OR && (flags & E_WALRUS)) {
        if (ls->used_walrus)
          throwerr(ls, "'or' invalidates previous usages of walrus operator", "due to the 'or', it is no longer guaranteed that the local will be initialized by the time it's in scope.");
        flags &= ~E_WALRUS;
        flags |= E_OR_KILLED_WALRUS;
      }
      nextop = subexpr(ls, &v2, priority[op].right, subexpr_prop, flags);
      luaK_posfix(ls->fs, op, v, &v2, line);
      if (canchainopr(op) && canchainopr(nextop)) {
        while (true) {
          op = nextop;
          if (v2.k == VNONRELOC && ls->fs->freereg == v2.u.reg) {
            ls->fs->freereg++;
          }
          luaX_next(ls);  /* skip operator */
          /* to generate 'a < b < c': 'a < b' is in `v`. 'b' is in `v2`. 'c' can be read from lexer state. */
          luaK_infix(ls->fs, OPR_AND, v);
          expdesc v3;
          luaK_infix(ls->fs, op, &v2);
          nextop = subexpr(ls, &v3, priority[op].right, subexpr_prop, flags);
          luaK_posfix(ls->fs, op, &v2, &v3, line);
          luaK_posfix(ls->fs, OPR_AND, v, &v2, line);
          if (!canchainopr(nextop))
            break;
          v2 = v3;
        }
      }
    }
    op = nextop;
  }
  leavelevel(ls);
  return op;  /* return first untreated operator */
}


static void expr (LexState *ls, expdesc *v, TypeHint *prop, int flags) {
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.pushLocals();
  }
#endif
  subexpr(ls, v, 0, prop, flags);
  if (testnext(ls, '?')) { /* ternary expression? */
    check_condition(ls, !(flags & E_NO_CONSUME_COLON), "cannot use a ternary expression in this context");
    if (prop) prop->clear(); /* we don't care what type the condition is/was */
    int escape = NO_JUMP;
    v->normalizeFalse();
    if (luaK_isalwaystrue(ls, v))
      throw_warn(ls, "unreachable code", "the condition before the '?' is always truthy, hence the expression after the ':' is never used.", WT_UNREACHABLE_CODE);
    else if (luaK_isalwaysfalse(ls, v))
      throw_warn(ls, "unreachable code", "the condition before the '?' is always falsy, hence the expression before the ':' is never used.", WT_UNREACHABLE_CODE);
    luaK_goiftrue(ls->fs, v);
    int condition = v->f;
    expr(ls, v, prop, E_NO_METHOD_CALL);
    auto fs = ls->fs;
    auto reg = luaK_exp2anyreg(fs, v);
    luaK_concat(fs, &escape, luaK_jump(fs));
    luaK_patchtohere(fs, condition);
    checknext(ls, ':');
    ls->pushContext(PARCTX_TERNARY_C);
    expr(ls, v, prop, flags & E_NO_METHOD_CALL);
    ls->popContext(PARCTX_TERNARY_C);
    luaK_exp2reg(fs, v, reg);
    luaK_patchtohere(fs, escape);
  }
}

/* }==================================================================== */



/*
** {======================================================================
** Rules for Statements
** =======================================================================
*/


static void block (LexState *ls, TypeHint *prop = nullptr) {
  /* block -> statlist */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, BlockType::BT_DEFAULT);
  statlist(ls, prop);
  leaveblock(fs);
}


/*
** structure to chain all variables in the left-hand side of an
** assignment
*/
struct LHS_assign {
  struct LHS_assign *prev, *next; /* previous & next lhs objects */
  expdesc v;  /* variable (global, local, upvalue, or indexed) */
};


/*
** check whether, in an assignment to an upvalue/local variable, the
** upvalue/local variable is begin used in a previous assignment to a
** table. If so, save original upvalue/local value in a safe place and
** use this safe copy in the previous assignment.
*/
static void check_conflict (LexState *ls, struct LHS_assign *lh, expdesc *v) {
  FuncState *fs = ls->fs;
  int extra = fs->freereg;  /* eventual position to save local variable */
  int conflict = 0;
  for (; lh; lh = lh->prev) {  /* check all previous assignments */
    if (vkisindexed(lh->v.k)) {  /* assignment to table field? */
      if (lh->v.k == VINDEXUP) {  /* is table an upvalue? */
        if (v->k == VUPVAL && lh->v.u.ind.t == v->u.info) {
          conflict = 1;  /* table is the upvalue being assigned now */
          lh->v.k = VINDEXSTR;
          lh->v.u.ind.t = extra;  /* assignment will use safe copy */
        }
      }
      else {  /* table is a register */
        if (v->k == VLOCAL && lh->v.u.ind.t == v->u.var.ridx) {
          conflict = 1;  /* table is the local being assigned now */
          lh->v.u.ind.t = extra;  /* assignment will use safe copy */
        }
        /* is index the local being assigned? */
        if (lh->v.k == VINDEXED && v->k == VLOCAL &&
            lh->v.u.ind.idx == v->u.var.ridx) {
          conflict = 1;
          lh->v.u.ind.idx = extra;  /* previous assignment will use safe copy */
        }
      }
    }
  }
  if (conflict) {
    /* copy upvalue/local value to a temporary (in position 'extra') */
    if (v->k == VLOCAL)
      luaK_codeABC(fs, OP_MOVE, extra, v->u.var.ridx, 0);
    else
      luaK_codeABC(fs, OP_GETUPVAL, extra, v->u.info, 0);
    luaK_reserveregs(fs, 1);
  }
}

/* 
  compound assignment function
  determines the binary operation to perform depending on lexer state tokens (ls->lasttoken)
  resets the lexer state token
  reserves N registers (where N = local variables on stack)
  preforms binary operation and assignment
*/ 
static void compoundassign (LexState *ls, expdesc *v, BinOpr op) {
  luaX_next(ls);
  int line = ls->getLineNumber();
  FuncState *fs = ls->fs;
  expdesc e = *v, v2;
  if (v->k != VLOCAL) {  /* complex lvalue, use a temporary register. linear perf incr. with complexity of lvalue */
    const auto regs_to_reserve = fs->freereg-fs->nactvar;
    luaK_dischargevars(fs, &e);
    luaK_reserveregs(fs, regs_to_reserve);
    enterlevel(ls);
    luaK_infix(fs, op, &e);
    expr(ls, &v2);
    luaK_posfix(fs, op, &e, &v2, line);
    leavelevel(ls);
    luaK_exp2nextreg(fs, &e);
    luaK_setoneret(ls->fs, &e);
    luaK_storevar(ls->fs, v, &e);
  }
  else {  /* simple lvalue; a local. directly change value (~20% speedup vs temporary register) */
    enterlevel(ls);
    luaK_infix(fs, op, &e);
    expr(ls, &v2);
    luaK_posfix(fs, op, &e, &v2, line);
    leavelevel(ls);
    luaK_setoneret(ls->fs, &e);
    luaK_storevar(ls->fs, v, &e);
  }
}

/*
  assignment function
  handles every Lua assignment
  special cases for compound operators via lexer state tokens (ls->t.seminfo.i)
*/
static void restassign (LexState *ls, struct LHS_assign *lh, int nvars) {
  int line = ls->getLineNumber(); /* in case we need to emit a warning */
  expdesc e;
  check_condition(ls, vkisvar(lh->v.k), "syntax error");
  check_readonly(ls, &lh->v);
  if (testnext(ls, ',')) {  /* restassign -> ',' suffixedexp restassign */
    struct LHS_assign nv;
    nv.prev = lh;
    nv.next = NULL;
    lh->next = &nv;
    suffixedexp(ls, &nv.v);
    if (!vkisindexed(nv.v.k))
      check_conflict(ls, lh, &nv.v);
    enterlevel(ls);  /* control recursion depth */
    restassign(ls, &nv, nvars+1);
    leavelevel(ls);
  }
  else {  /* restassign -> '=' explist */
    if (ls->t.token != TK_NE)
      check(ls, '=');
    BinOpr compound_op = (ls->t.token == TK_NE ? OPR_BXOR : getbinopr((int)ls->t.seminfo.i));
    if (compound_op != OPR_NOBINOPR) {  /* compound operator? */
      if (l_unlikely(ls->t.seminfo.i == TK_POW))
        throw_warn(ls, "'**' is deprecated", "use '^' instead", WT_DEPRECATED);
      check_condition(ls, nvars == 1, "unsupported tuple assignment");
      compoundassign(ls, &lh->v, compound_op);  /* perform binop & assignment */
      return;  /* avoid default */
    }
    luaX_next(ls);
    TypeHint prop{};
    ParserContext ctx = ((nvars == 1) ? PARCTX_CREATE_VAR : PARCTX_CREATE_VARS);
    ls->pushContext(ctx);
    int nexps = explist(ls, &e, &prop);
    ls->popContext(ctx);
    if (nexps != nvars)
      adjust_assign(ls, nvars, nexps, &e);
    else {
      luaK_setoneret(ls->fs, &e);  /* close last expression */
      if (lh->v.k == VINDEXUP) {
        TValue *key = &ls->fs->f->k[lh->v.u.ind.idx];
        lua_assert(ttype(key) == LUA_TSTRING);
        get_global_prop(ls, tsvalue(key)).merge(prop);
      }
      if (lh->v.k == VLOCAL) { /* assigning to a local variable? */
        exp_propagate(ls, e, prop);
        process_assign(ls, lh->v.u.var.vidx, prop, line);
      }
      luaK_storevar(ls->fs, &lh->v, &e);
      return;  /* avoid default */
    }
  }
  init_exp(&e, VNONRELOC, ls->fs->freereg-1);  /* default assignment */
  luaK_storevar(ls->fs, &lh->v, &e);
}

static int cond (LexState *ls, bool for_while_loop) {
  /* cond -> exp */
  expdesc v;
  ls->used_walrus = false;
  expr(ls, &v, nullptr, for_while_loop * E_WALRUS);  /* read condition */
  v.normalizeFalse();
  luaK_goiftrue(ls->fs, &v);
  return v.f;
}


static void gotostat (LexState *ls) {
  const auto line = ls->getLineNumber();
  lgoto(ls, str_checkname(ls, N_RESERVED), line);
}


static void enumstat (LexState *ls) {
  /* enumstat -> ENUM [[CLASS] NAME] BEGIN NAME ['=' INT] { ',' NAME ['=' INT] } END */

  luaX_next(ls); /* skip 'enum' */

  EnumDesc *ed = nullptr;
  bool is_enum_class = false;
  if (gett(ls) != TK_BEGIN && gett(ls) != TK_DO) { /* enum has name (and possibly modifier)? */
    if (ls->t.token == TK_CLASS
      || (ls->t.token == TK_NAME && strcmp(getstr(ls->t.seminfo.ts), "class") == 0)
      ) {
      is_enum_class = true;
      luaX_next(ls);
    }
    auto vidx = new_localvar(ls, str_checkname(ls, 0), ls->getLineNumber());
    auto var = getlocalvardesc(ls->fs, vidx);
    var->vd.kind = RDKENUM;
    setivalue(&var->k, ls->enums.size());
    ed = &ls->enums.emplace_back(EnumDesc{});
    ls->fs->nactvar++;
  }

  const auto line_begin = ls->getLineNumber();
  if (!testnext(ls, TK_DO))
    checknext(ls, TK_BEGIN);

  lua_Integer i = 1;
  while (gett(ls) != TK_END) {
    TString *name = str_checkname(ls, 0);
    int vidx;
    if (!is_enum_class) {
      vidx = new_localvar(ls, name, ls->getLineNumber());
    }
    if (testnext(ls, '=')) {
      expdesc v;
      expr(ls, &v);
      if (v.k == VCONST) { /* compile-time constant? */
        TValue *k = &ls->dyd->actvar.arr[v.u.info].k;
        if (ttype(k) == LUA_TNUMBER && ttisinteger(k)) { /* integer value? */
          init_exp(&v, VKINT, 0);
          v.u.ival = ivalue(k);
        }
      }
      if (v.k != VKINT) { /* assert expdesc kind */
        throwerr(ls, "expected integer constant", "unexpected expression type");
      }
      i = v.u.ival;
    }
    if (ed) {
      ed->enumerators.emplace_back(EnumDesc::Enumerator{ name, i });
    }
    if (!is_enum_class) {
      auto var = getlocalvardesc(ls->fs, vidx);
      var->vd.kind = RDKCTC;
      setivalue(&var->k, i);
      ls->fs->nactvar++;
    }
    i++;
    if (gett(ls) != ',') break;
    luaX_next(ls);
  }

  check_match(ls, TK_END, TK_ENUM, line_begin);
}


/*
** Check whether there is already a label with the given 'name'.
*/
static void checkrepeated (LexState *ls, TString *name) {
  Labeldesc *lb = findlabel(ls, name);
  if (l_unlikely(lb != NULL)) {  /* already defined? */
    const char *msg = "label '%s' already defined on line %d";
    msg = luaO_pushfstring(ls->L, msg, getstr(name), lb->line);
    luaK_semerror(ls, msg);  /* error */
  }
}


static void labelstat (LexState *ls, TString *name, int line) {
  /* label -> '::' NAME '::' */
  checknext(ls, TK_DBCOLON);  /* skip double colon */
  while (ls->t.token == ';' || ls->t.token == TK_DBCOLON)
    statement(ls);  /* skip other no-op statements */
  checkrepeated(ls, name);  /* check for repeated labels */
  createlabel(ls, name, line, block_follow(ls, 0), false);
}


static void whilestat (LexState *ls, int line) {
  /* whilestat -> WHILE cond DO block END */
  FuncState *fs = ls->fs;
  int whileinit;
  int condexit;
  BlockCnt bl;
  BlockCnt innerbl;

  luaX_next(ls);  /* skip WHILE */
  whileinit = luaK_getlabel(fs);
  enterblock(fs, &bl, BlockType::BT_BREAK);
  enterblock(fs, &innerbl, BlockType::BT_CONTINUE);
  condexit = cond(ls, true);
  checknext(ls, TK_DO);
  statlist(ls);
  leaveblock(fs);
  luaK_jumpto(fs, whileinit);
  check_match(ls, TK_END, TK_WHILE, line);
  leaveblock(fs);
  luaK_patchtohere(fs, condexit);  /* false conditions finish the loop */
}


static void repeatstat (LexState *ls) {
  /* repeatstat -> REPEAT block ( UNTIL | WHEN ) cond */
  int condexit;
  FuncState *fs = ls->fs;
  int repeat_init = luaK_getlabel(fs);
  BlockCnt bl1, bl2;
  enterblock(fs, &bl1, BlockType::BT_BREAK);  /* loop block */
  enterblock(fs, &bl2, BlockType::BT_CONTINUE);  /* scope block */
  luaX_next(ls);  /* skip REPEAT */
  statlist(ls);
  createlabel(ls, &bl2, 0, 0, true);
  bl2.type = BlockType::BT_DEFAULT;
  checknext(ls, TK_UNTIL);
  condexit = cond(ls);  /* read condition (inside scope block) */
  leaveblock(fs);  /* finish scope */
  if (bl2.upval) {  /* upvalues? */
    int exit = luaK_jump(fs);  /* normal exit must jump over fix */
    luaK_patchtohere(fs, condexit);  /* repetition must close upvalues */
    luaK_codeABC(fs, OP_CLOSE, reglevel(fs, bl2.nactvar), 0, 0);
    condexit = luaK_jump(fs);  /* repeat after closing upvalues */
    luaK_patchtohere(fs, exit);  /* normal exit comes to here */
  }
  luaK_patchlist(fs, condexit, repeat_init);  /* close the loop */
  leaveblock(fs);  /* finish loop */
}


/*
** Read an expression and generate code to put its results in next
** stack slot.
**
*/
static void exp1 (LexState *ls) {
  expdesc e;
  expr(ls, &e);
  luaK_exp2nextreg(ls->fs, &e);
  lua_assert(e.k == VNONRELOC);
}


/*
** Fix for instruction at position 'pc' to jump to 'dest'.
** (Jump addresses are relative in Lua). 'back' true means
** a back jump.
*/
static void fixforjump (FuncState *fs, int pc, int dest, int back) {
  Instruction *jmp = &fs->f->code[pc];
  int offset = dest - (pc + 1);
  if (back)
    offset = -offset;
  if (l_unlikely(offset > MAXARG_Bx))
    luaX_syntaxerror(fs->ls, "control structure too long");
  SETARG_Bx(*jmp, offset);
}


/*
** Generate code for a 'for' loop.
*/
static void forbody (LexState *ls, int base, int line, int nvars, int isgen, TypeHint *prop) {
  /* forbody -> DO block */
  static const OpCode forprep[2] = {OP_FORPREP, OP_TFORPREP};
  static const OpCode forloop[2] = {OP_FORLOOP, OP_TFORLOOP};
  BlockCnt bl;
  FuncState *fs = ls->fs;
  int prep, endfor;
  checknext(ls, TK_DO);
  prep = luaK_codeABx(fs, forprep[isgen], base, 0);
  enterblock(fs, &bl, BlockType::BT_CONTINUE);  /* scope for declared variables */
  adjustlocalvars(ls, nvars);
  luaK_reserveregs(fs, nvars);
  block(ls, prop);
  leaveblock(fs);  /* end of scope for declared variables */
  fixforjump(fs, prep, luaK_getlabel(fs), 0);
  if (isgen) {  /* generic for? */
    luaK_codeABC(fs, OP_TFORCALL, base, 0, nvars);
    luaK_fixline(fs, line);
  }
  endfor = luaK_codeABx(fs, forloop[isgen], base, 0);
  fixforjump(fs, endfor, prep + 1, 1);
  luaK_fixline(fs, line);
}


static void fornum (LexState *ls, TString *varname, TypeHint *prop, int line) {
  /* fornum -> NAME = exp,exp[,exp] forbody */
  FuncState *fs = ls->fs;
  int base = fs->freereg;
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvar(ls, varname);
  checknext(ls, '=');
  exp1(ls);  /* initial value */
  checknext(ls, ',');
  exp1(ls);  /* limit */
  if (testnext(ls, ','))
    exp1(ls);  /* optional step */
  else {  /* default step = 1 */
    luaK_int(fs, fs->freereg, 1);
    luaK_reserveregs(fs, 1);
  }
  adjustlocalvars(ls, 3);  /* control variables */
  forbody(ls, base, line, 1, 0, prop);
}


static void forlist (LexState *ls, TString *indexname, TypeHint *prop) {
  /* forlist -> NAME {,NAME} IN explist forbody */
  FuncState *fs = ls->fs;
  expdesc e;
  int nvars = 5;  /* gen, state, control, toclose, 'indexname' */
  int line;
  int base = fs->freereg;
  /* create control variables */
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  /* create declared variables */
  new_localvar(ls, indexname);
  while (testnext(ls, ',')) {
    new_localvar(ls, str_checkname(ls));
    nvars++;
  }
  checknext(ls, TK_IN);
  line = ls->getLineNumber();
  adjust_assign(ls, 4, explist(ls, &e), &e);
  adjustlocalvars(ls, 4);  /* control variables */
  marktobeclosed(fs);  /* last control var. must be closed */
  luaK_checkstack(fs, 3);  /* extra space to call generator */
  forbody(ls, base, line, nvars - 4, 1, prop);
}


static void forvlist (LexState *ls, TypeHint *prop) {
  /* forvlist -> explist AS NAME forbody */
  FuncState *fs = ls->fs;
  expdesc e;
  int nvars = 5;  /* gen, state, control, toclose, 'indexname' */
  int line;
  int base = fs->freereg;
  /* create control variables */
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  /* create variable for key */
  new_localvar(ls, luaS_newliteral(ls->L, "(for state)"));
  /* create variable for value */
  int vidx = new_localvar(ls, luaS_newliteral(ls->L, "(for state)"));
  nvars++;

  line = ls->getLineNumber();
  adjust_assign(ls, 4, explist(ls, &e), &e);
  adjustlocalvars(ls, 4);  /* control variables */
  marktobeclosed(fs);  /* last control var. must be closed */
  luaK_checkstack(fs, 3);  /* extra space to call generator */

  checknext(ls, TK_AS);
  TString *name = str_checkname(ls);
  checkforshadowing(ls, fs, name, line);
  getlocalvardesc(fs, vidx)->vd.name = name;

  forbody(ls, base, line, nvars - 4, 1, prop);
}


static void forstat (LexState *ls, int line, TypeHint *prop) {
  /* forstat -> FOR (fornum | forlist) END */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, BlockType::BT_BREAK);  /* scope for loop and control variables */
  luaX_next(ls);  /* skip 'for' */

  if (luaX_lookahead(ls) == '=') {
    TString *varname = str_checkname(ls);  /* first variable name */
    fornum(ls, varname, prop, line);
  }
  else if (luaX_lookahead(ls) == ',' || luaX_lookahead(ls) == TK_IN) {
    TString *varname = str_checkname(ls);  /* first variable name */
    forlist(ls, varname, prop);
  }
  else {
    forvlist(ls, prop);
  }

  check_match(ls, TK_END, TK_FOR, line);
  leaveblock(fs);  /* loop scope ('break' jumps to this point) */
}


static void test_then_block (LexState *ls, int *escapelist, TypeHint *prop) {
  /* test_then_block -> [IF | ELSEIF] cond THEN block */
  BlockCnt bl;
  FuncState *fs = ls->fs;
  expdesc v;
  int jf;  /* instruction to skip 'then' code (if condition is false) */
  luaX_next(ls);  /* skip IF or ELSEIF */

  if (ls->t.token == TK_NAME && eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "type"))) {
    luaX_next(ls);
    if (testnext(ls, '(')) {
      if (isnametkn(ls)) {
        expdesc var;
        singlevar(ls, &var);
        if (var.k == VLOCAL && testnext(ls, ')')) {
          if (testnext(ls, TK_EQ)) {
            if (ls->t.token == TK_STRING) {
              bl.var_overide_vidx = var.u.var.vidx;
              if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "nil"))) {
                bl.var_overide = VT_NIL;
              }
              else if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "boolean"))) {
                bl.var_overide = VT_BOOL;
              }
              else if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "number"))) {
                bl.var_overide = VT_NUMBER;
              }
              else if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "string"))) {
                bl.var_overide = VT_STR;
              }
              else if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "table"))) {
                bl.var_overide = VT_TABLE;
              }
              else if (eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "function"))) {
                bl.var_overide = VT_FUNC;
              }
              else if (!eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "userdata"))
                && !eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "thread"))
                && !eqstr(ls->t.seminfo.ts, luaX_newliteral(ls, "no value"))
              ) {
                throw_warn(ls, luaO_fmt(ls->L, "'%s' is not a possible return value of 'type'", getstr(ls->t.seminfo.ts)), WT_POSSIBLE_TYPO);
                ls->L->top.p--;
              }
            }
            luaX_prev(ls);  /* back to '==' */
          }
          luaX_prev(ls);  /* back to ')' */
        }
        luaX_prev(ls);  /* back to nametkn */
      }
      luaX_prev(ls);  /* back to '(' */
    }
    luaX_prev(ls);  /* back to "type" */
  }

  ls->used_walrus = false;
  expr(ls, &v, nullptr, E_WALRUS);  /* read condition */
  const bool alwaystrue = luaK_isalwaystrue(ls, &v);
  if (luaK_isalwaysfalse(ls, &v))
    throw_warn(ls, "unreachable code", "this condition will never be truthy.", WT_UNREACHABLE_CODE);
  if (testnext(ls, TK_THEN)) {
    /* standard block opener for ifstat */
  }
  else if (testnext(ls, TK_DO)) {
    throw_warn(ls, "non-portable block opener", "using 'do' instead of 'then' is Pluto-specific", WT_NON_PORTABLE_CODE);
  }
  else {
    const char* token = luaX_token2str(ls, ls->t);
    throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "expected 'then' or 'do' to open the block");
  }
  if (ls->t.token == TK_BREAK && luaX_lookahead(ls) != TK_INT) {  /* 'if x then break' and not 'if x then break int' ? */
    ls->laststat.token = TK_BREAK;
    int line = ls->getLineNumber();
    luaK_goiffalse(ls->fs, &v);  /* will jump if condition is true */
    luaX_next(ls);  /* skip 'break' */
    enterblock(fs, &bl, BlockType::BT_DEFAULT);  /* must enter block before 'goto' */
    lbreak(ls, 1, line, v.t);
    // TODO: Allow the integer level for break statements
    while (testnext(ls, ';')) {}  /* skip semicolons */
    if (block_follow(ls, 0)) {  /* jump is the entire block? */
      leaveblock(fs);
      return;  /* and that is it */
    }
    else  /* must skip over 'then' part if condition is false */
      jf = luaK_jump(fs);
  }
  else {  /* regular case (not a break) */
    luaK_goiftrue(ls->fs, &v);  /* skip over block if condition is false */
    enterblock(fs, &bl, BlockType::BT_DEFAULT);
    jf = v.f;
  }
  statlist(ls, prop);  /* 'then' part */
  leaveblock(fs);
  if (ls->t.token == TK_ELSE ||
      ls->t.token == TK_ELSEIF) {  /* followed by 'else'/'elseif'? */
    luaK_concat(fs, escapelist, luaK_jump(fs));  /* must jump over it */
    if (alwaystrue)
      throw_warn(ls, "unreachable code", "the condition in the if block is always truthy, hence this else block is unreachable.", WT_UNREACHABLE_CODE);
  }
  luaK_patchtohere(fs, jf);
}


static void ifstat (LexState *ls, int line, TypeHint *prop = nullptr) {
  /* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
  FuncState *fs = ls->fs;
  BlockCnt walrusbl;
  enterblock(fs, &walrusbl, BlockType::BT_DEFAULT);
  int escapelist = NO_JUMP;  /* exit list for finished parts */
  test_then_block(ls, &escapelist, prop);  /* IF cond THEN block */
  while (ls->t.token == TK_ELSEIF) {
    leaveblock(fs);
    enterblock(fs, &walrusbl, BlockType::BT_DEFAULT);
    test_then_block(ls, &escapelist, prop);  /* ELSEIF cond THEN block */
  }
  if (testnext(ls, TK_ELSE)) {
    leaveblock(fs);
    enterblock(fs, &walrusbl, BlockType::BT_DEFAULT);
    if (ls->t.token == TK_IF && ls->t.line == luaX_lookbehind(ls).line)
      ls->else_if = ls->getLineNumber();
    block(ls, prop);  /* 'else' part */
  }
  check_match(ls, TK_END, TK_IF, line);
  luaK_patchtohere(fs, escapelist);  /* patch escape list to 'if' end */
  leaveblock(fs);
}


/* keep advancing until we hit non-nested '$else', '$elseif' or '$end' */
static void skip_constexpr_block (LexState *ls) {
  int depth = 0;
  while (ls->t.token != TK_EOS) {
    if (ls->t.token == '$') {
      const auto la_tk = luaX_lookahead(ls);
      if (la_tk == TK_IF) {
        ++depth;
      }
      else if (la_tk == TK_ELSE || la_tk == TK_ELSEIF) {
        if (depth == 0) {
          break;
        }
      }
      else if (la_tk == TK_END) {
        if (depth == 0) {
          break;
        }
        depth--;
      }
      luaX_next(ls);
    }
    luaX_next(ls);
  }
}

static void constexprifstat (LexState *ls, int line) {
  bool hit = false;

  expdesc c;
  expr(ls, &c);
  if (!testnext(ls, TK_DO))
    checknext(ls, TK_THEN);
  if (luaK_isalwaystrue(ls, &c)) {
    hit = true;
    block(ls);
  }
  else {
    if (!luaK_isalwaysfalse(ls, &c))
      throwerr(ls, "this condition cannot be evaluated at compile-time", "");
    skip_constexpr_block(ls);
  }

  while (checknext(ls, '$'), testnext(ls, TK_ELSEIF)) {
    expr(ls, &c);
    if (!testnext(ls, TK_DO))
      checknext(ls, TK_THEN);
    if (luaK_isalwaystrue(ls, &c)) {
      if (hit) {
        skip_constexpr_block(ls);
      }
      else {
        hit = true;
        block(ls);
      }
    }
    else {
      if (!luaK_isalwaysfalse(ls, &c))
        throwerr(ls, "this condition cannot be evaluated at compile-time", "");
      skip_constexpr_block(ls);
    }
  }

  if (testnext(ls, TK_ELSE)) {
    if (!hit) {
      block(ls);
    }
    else {
      skip_constexpr_block(ls);
    }
    checknext(ls, '$');
  }

  check_match(ls, TK_END, TK_IF, line);
}


static void constexprdefinestat (LexState *ls, int line) {
  FuncState *fs = ls->fs;
  int vidx = new_localvar(ls, str_checkname(ls, N_OVERRIDABLE), line);
  TypeHint hint = gettypehint(ls);
  Vardesc *var = getlocalvardesc(fs, vidx);
  var->vd.kind = RDKCTC;
  *var->vd.hint = hint;

  expdesc e;
  checknext(ls, '=');
  ls->pushContext(PARCTX_CREATE_VAR);
  TypeHint t;
  expr_propagate(ls, &e, t);
  ls->popContext(PARCTX_CREATE_VAR);
  if (!luaK_exp2const(fs, &e, &var->k))
    throwerr(ls, "variable was not assigned a compile-time constant value", "expression not constant", line);

  fs->nactvar++;  /* don't adjustlocalvars, but count it */
}


static void constexprstat (LexState *ls, int line) {
  if (testnext(ls, TK_IF)) {
    constexprifstat(ls, line);
  }
  else if (ls->t.token == TK_NAME && strcmp(getstr(ls->t.seminfo.ts), "define") == 0) {
    luaX_next(ls);  /* skip 'define' */
    constexprdefinestat(ls, line);
  }
  else {
    const char *token = luaX_token2str(ls, ls->t);
    throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
  }
}


static void localfunc (LexState *ls) {
  expdesc b;
  FuncState *fs = ls->fs;
  int fvar = fs->nactvar;  /* function's variable index */
  new_localvar(ls, str_checkname(ls, N_OVERRIDABLE));  /* new local variable */
  adjustlocalvars(ls, 1);  /* enter its scope */
  TypeDesc funcdesc;
  body(ls, &b, 0, ls->getLineNumber(), &funcdesc);  /* function created in next register */
  getlocalvardesc(fs, fvar)->vd.prop->emplaceTypeDesc(std::move(funcdesc));
  /* debug information will only see the variable after this point! */
  localdebuginfo(fs, fvar)->startpc = fs->pc;
}


static int getlocalattribute (LexState *ls) {
  /* ATTRIB -> ['<' Name '>'] */
  if (testnext(ls, '<')) {
    TString *ts = str_checkname(ls);
    const char *attr = getstr(ts);
    checknext(ls, '>');
    if (strcmp(attr, "const") == 0)
      return RDKCONST;  /* read-only variable */
    else if (strcmp(attr, "constexpr") == 0) {
      throw_warn(ls, "the 'constexpr' attribute will be removed in future versions of Pluto.", "use the 'const' attribute or '$define' statement, instead.", WT_DEPRECATED);
      return RDKCONSTEXP;  /* read-only variable */
    }
    else if (strcmp(attr, "close") == 0)
      return RDKTOCLOSE;  /* to-be-closed variable */
    else {
      luaX_prev(ls); // back to '>'
      luaX_prev(ls); // back to attribute
      luaK_semerror(ls,
        luaO_pushfstring(ls->L, "unknown attribute '%s'", attr));
    }
  }
  return VDKREG;  /* regular variable */
}


static void checktoclose (FuncState *fs, int level) {
  if (level != -1) {  /* is there a to-be-closed variable? */
    marktobeclosed(fs);
    luaK_codeABC(fs, OP_TBC, reglevel(fs, level), 0, 0);
  }
}


static void restdestructuring (LexState *ls, int line, std::vector<std::pair<TString*, expdesc>>& pairs) {
  checknext(ls, '=');

  /* begin scope of the locals we want to create */
  for (const auto& p : pairs) {
    new_localvar(ls, p.first, line);
  }
  expdesc e;
  e.k = VVOID;
  adjust_assign(ls, (int)pairs.size(), 0, &e);
  adjustlocalvars(ls, (int)pairs.size());

  /* get table */
  expdesc t;
  expr(ls, &t);

  /* ensure table has a place to stay */
  TString *temporary = nullptr;
  if (t.k != VLOCAL) {
    if (pairs.size() == 1) {
      luaK_exp2anyreg(ls->fs, &t);
    }
    else {
      temporary = luaS_newliteral(ls->L, "(temporary)");
      new_localvar(ls, temporary, line);
      adjust_assign(ls, 1, 1, &t);
      adjustlocalvars(ls, 1);
    }
  }

  /* assign locals */
  for (auto& p : pairs) {
    expdesc e, l;
    if (temporary)
      singlevar(ls, &e, temporary);
    else
      e = t;
    luaK_indexed(ls->fs, &e, &p.second);
    singlevar(ls, &l, p.first);
    luaK_storevar(ls->fs, &l, &e);
  }

  /* release table */
  if (temporary) {
    removevars(ls->fs, ls->fs->nactvar - 1);
  }
}

static void destructuring (LexState *ls) {
  auto line = ls->getLineNumber();
  std::vector<std::pair<TString*, expdesc>> pairs{};
  luaX_next(ls); /* skip '{' */
  do {
    TString* var = str_checkname(ls, N_OVERRIDABLE);
    TString* prop = var;
    if (testnext(ls, '='))
      prop = str_checkname(ls);
    expdesc propexp;
    codestring(&propexp, prop);
    pairs.emplace_back(var, std::move(propexp));
  } while (testnext(ls, ','));
  check_match(ls, '}', '{', line);
  restdestructuring(ls, line, pairs);
}

static void arraydestructuring (LexState *ls) {
  auto line = ls->getLineNumber();
  std::vector<std::pair<TString*, expdesc>> pairs{};
  luaX_next(ls); /* skip '[' */
  expdesc prop;
  init_exp(&prop, VKINT, 0);
  prop.u.ival = 1;
  do {
    pairs.emplace_back(str_checkname(ls, N_OVERRIDABLE), prop);
    prop.u.ival++;
  } while (testnext(ls, ','));
  check_match(ls, ']', '[', line);
  restdestructuring(ls, line, pairs);
}


static void localstat (LexState *ls) {
  if (ls->t.token == '{') {
    destructuring(ls);
    return;
  }
  if (ls->t.token == '[') {
    arraydestructuring(ls);
    return;
  }
  /* stat -> LOCAL NAME ATTRIB { ',' NAME ATTRIB } ['=' explist] */
  FuncState *fs = ls->fs;
  int toclose = -1;  /* index of to-be-closed variable (if any) */
  Vardesc *var;  /* last variable */
  int vidx, kind;  /* index and kind of last variable */
  int nvars = 0;
  int nexps;
  expdesc e;
  auto line = ls->getLineNumber(); /* in case we need to emit a warning */
  size_t starting_tidx = ls->tidx; /* code snippets on tuple assignments can have inaccurate line readings because the parser skips lines until it can close the statement */
  bool is_constexpr = false;
  ls->localstat_variable_names.clear();
  ls->localstat_expression_names.clear();
  do {
    if (is_constexpr)
      luaK_semerror(ls, "<constexpr> must only be used on the last variable in local list");
    TString* name = str_checkname(ls, N_OVERRIDABLE);
    vidx = new_localvar(ls, name, line, gettypehint(ls), false);
    ls->localstat_variable_names.emplace(name);
    kind = getlocalattribute(ls);
    var = getlocalvardesc(fs, vidx);
    var->vd.kind = kind;
    if (kind == RDKTOCLOSE) {  /* to-be-closed? */
      if (toclose != -1) { /* one already present? */
        luaX_setpos(ls, starting_tidx);
        luaK_semerror(ls, "multiple to-be-closed variables in local list");
      }
      toclose = fs->nactvar + nvars;
    }
    else if (kind == RDKCONSTEXP) {
      is_constexpr = true;
    }
    else if (kind == VDKREG) {
      if (line == ls->getLineNumber() && ls->t.token == TK_NAME) {
        if (strcmp(getstr(ls->t.seminfo.ts), "const") == 0
          || strcmp(getstr(ls->t.seminfo.ts), "constexpr") == 0
          || strcmp(getstr(ls->t.seminfo.ts), "close") == 0
          ) {
          throw_warn(ls, "possibly mistyped attribute", luaO_fmt(ls->L, "did you mean '<%s>'?", getstr(ls->t.seminfo.ts)), WT_POSSIBLE_TYPO);
          ls->L->top.p--;
        }
      }
    }
    nvars++;
  } while (testnext(ls, ','));
  ls->localstat_ts.clear();
  if (testnext(ls, '=')) {
    ParserContext ctx = ((nvars == 1) ? PARCTX_CREATE_VAR : PARCTX_CREATE_VARS);
    ls->pushContext(ctx);
    nexps = 1;
    expr_propagate_warn(ls, &e, *(TypeHint*)ls->localstat_ts.emplace_back(new_typehint(ls)), ls->localstat_expression_names);
    while (testnext(ls, ',')) {
      luaK_exp2nextreg(ls->fs, &e);
      expr_propagate_warn(ls, &e, *(TypeHint*)ls->localstat_ts.emplace_back(new_typehint(ls)), ls->localstat_expression_names);
      nexps++;
    }
    ls->popContext(ctx);
    for (auto variable_name : ls->localstat_variable_names) {
      if (ls->localstat_expression_names.find(variable_name) == ls->localstat_expression_names.end()) { // Not a localization optimization?
        checkforshadowing(ls, fs, variable_name, line, true, false); // new_localvar already checked for local duplication
      }
    }
    var = getlocalvardesc(fs, vidx);  /* actvar array may have been relocated */
  }
  else {
    e.k = VVOID;
    nexps = 0;
    process_assign(ls, vidx, TypeHint{ VT_NIL }, line);
    checkforshadowing(ls, fs, ls->localstat_variable_names, line);
  }
  if (is_constexpr) {
    if (nvars != nexps) {
      luaX_prev(ls);
      luaK_semerror(ls, "<constexpr> variable assignment needs adjustment");
    }
    if (!vkisconst(e.k))
      throwerr(ls, "<constexpr> variable was not assigned a compile-time constant value", "expression not constant", line);
    var->vd.kind = RDKCONST;
  }
  if (nvars == nexps) { /* no adjustments? */
    if (var->vd.kind == RDKCONST &&  /* last variable is const? */
        luaK_exp2const(fs, &e, &var->k)) {  /* compile-time constant? */
      var->vd.kind = RDKCTC;  /* variable is a compile-time constant */
      adjustlocalvars(ls, nvars - 1);  /* exclude last variable */
      fs->nactvar++;  /* but count it */
    }
    else {
      vidx = vidx - nvars + 1;
      for (void* t : ls->localstat_ts) {
        process_assign(ls, vidx, *(TypeHint*)t, line);
        ++vidx;
      }
      adjust_assign(ls, nvars, nexps, &e);
      adjustlocalvars(ls, nvars);
    }
  }
  else {
    adjust_assign(ls, nvars, nexps, &e);
    adjustlocalvars(ls, nvars);
  }
  checktoclose(fs, toclose);
}

static void conststat (LexState *ls) {
  FuncState *fs = ls->fs;
  auto line = ls->getLineNumber(); /* in case we need to emit a warning */
  int vidx = new_localvar(ls, str_checkname(ls, N_OVERRIDABLE), line);
  TypeHint hint = gettypehint(ls);
  Vardesc *var = getlocalvardesc(fs, vidx);
  var->vd.kind = RDKCONST;
  *var->vd.hint = hint;

  expdesc e;
  if (testnext(ls, '=')) {
    ls->pushContext(PARCTX_CREATE_VAR);
    TypeHint t;
    expr_propagate(ls, &e, t);
    ls->popContext(PARCTX_CREATE_VAR);
    if (luaK_exp2const(fs, &e, &var->k)) {  /* compile-time constant? */
      var->vd.kind = RDKCTC;  /* variable is a compile-time constant */
      fs->nactvar++;  /* don't adjustlocalvars, but count it */
    }
    else {
      exp_propagate(ls, e, t);
      process_assign(ls, vidx, t, line);
      adjust_assign(ls, 1, 1, &e);
      adjustlocalvars(ls, 1);
    }
  }
  else {
    e.k = VVOID;
    process_assign(ls, vidx, TypeHint{ VT_NIL }, line);
    adjust_assign(ls, 1, 0, &e);
    adjustlocalvars(ls, 1);
  }
}


static int funcname (LexState *ls, expdesc *v) {
  /* funcname -> NAME {fieldsel} [':' NAME] */
  int ismethod = 0;
  singlevar(ls, v);
  while (ls->t.token == '.')
    fieldsel(ls, v);
  if (ls->t.token == ':') {
    ismethod = 1;
    fieldsel(ls, v);
  }
  return ismethod;
}


static void funcstat (LexState *ls, int line, const bool global) {
  /* funcstat -> FUNCTION funcname body */
  int ismethod;
  expdesc v, b;
  ismethod = funcname(ls, &v);
  if (!global)
    check_assignment(ls, &v);
  body(ls, &b, ismethod, line);
  check_readonly(ls, &v);
  luaK_storevar(ls->fs, &v, &b);
  luaK_fixline(ls->fs, line);  /* definition "happens" in the first line */
}


static void exprstat (LexState *ls) {
  /* stat -> func | assignment */
  FuncState *fs = ls->fs;
  struct LHS_assign v;
  suffixedexp(ls, &v.v);
  if (ls->t.token == '=' || ls->t.token == ',' || ls->t.token == TK_NE) { /* stat -> assignment ? */
    v.prev = NULL;
    check_assignment(ls, &v.v);
    restassign(ls, &v, 1);
  }
  else {  /* stat -> func */
    Instruction *inst;
    if (l_unlikely(v.v.k != VCALL && v.v.k != VSAFECALL)) {
      if (luaX_lookbehind(ls).token == TK_NAME) {
        if (auto t = find_non_compat_tkn_by_name(ls, getstr(luaX_lookbehind(ls).seminfo.ts))) {
          if (ls->getKeywordState(t) == KS_DISABLED_BY_PLUTO_INFORMED) {
            const auto entry = ls->uninformed_reserved.find(t);
            const auto line = entry == ls->uninformed_reserved.end() ? -1 : entry->second;
            throwerr(ls,
              luaO_fmt(ls->L, "syntax error near %s", luaX_token2str(ls, ls->t)),
              luaO_fmt(ls->L, "%s was not recognized as a statement because it was used as an identifier on line %d", luaX_token2str(ls, t), line)
            );
          }
        }
      }
      luaX_syntaxerror(ls, "syntax error");
    }
    inst = &getinstruction(fs, &v.v);
    SETARG_C(*inst, 1);  /* call statement uses no results */
    if (ls->nodiscard) {
      luaX_prev(ls);
      throw_warn(ls, "discarding return value of function declared '<nodiscard>'", WT_DISCARDED_RETURN);
      luaX_next(ls);
    }
  }
}


static void retstat (LexState *ls, TypeHint *prop) {
  /* stat -> RETURN [explist] [';'] */
  FuncState *fs = ls->fs;
  expdesc e;
  int nret;  /* number of values being returned */
  int first = luaY_nvarstack(fs);  /* first slot to be returned */
  int startpc = fs->pc;
  int endpc = -1;
  if (block_follow(ls, 1) || ls->t.token == ';'
    || (!ls->switchstates.empty() && (ls->t.token == TK_CASE || ls->t.token == TK_DEFAULT || ls->t.token == TK_BREAK))
  ) {
    nret = 0;  /* return no values */
    if (prop) prop->emplaceTypeDesc(VT_VOID);
  }
  else {
    if (fs->istrybody) {
      singlevar(ls, &e, luaX_newliteral(ls, "table"));
      luaK_exp2anyregup(ls->fs, &e);
      expdesc key;
      codestring(&key, luaX_newliteral(ls, "pack"));
      luaK_indexed(ls->fs, &e, &key);
      luaK_exp2nextreg(ls->fs, &e);
      lua_assert(first == e.u.reg);
      endpc = fs->pc-1;
      lua_assert(endpc >= startpc);
    }
    nret = explist(ls, &e, prop);  /* optional return values */
    if (prop && prop->empty())
      prop->emplaceTypeDesc(VT_DUNNO);  /* we are returning something, but we don't know what. (this is needed for trystat.) */
    if (hasmultret(e.k)) {
      luaK_setmultret(fs, &e);
      if (e.k == VCALL && nret == 1 && !fs->bl->insidetbc && !fs->istrybody) {  /* tail call? */
        SET_OPCODE(getinstruction(fs,&e), OP_TAILCALL);
        lua_assert(GETARG_A(getinstruction(fs,&e)) == luaY_nvarstack(fs));
      }
      nret = LUA_MULTRET;  /* return all values */
    }
    else {
      if (nret == 1 && !fs->istrybody) /* only one single value? */
        first = luaK_exp2anyreg(fs, &e);  /* can use original slot */
      else {  /* values must go to the top of the stack */
        luaK_exp2nextreg(fs, &e);
        lua_assert(nret + fs->istrybody == fs->freereg - first);
      }
    }
  }
  fs->seenrets |= 1 << (nret >= 0 && nret <= 3 ? nret : 3);
  if (fs->istrybody) {
    if (nret == 0) {
      init_exp(&e, VKINT, 0);
      e.u.ival = 0;
      first = luaK_exp2anyreg(fs, &e);  /* can use original slot */
      nret = 1;
    } else if (nret == 1 || nret == 2) {
      fs->f->code[endpc] = CREATE_ABx(OP_LOADI, first, nret + OFFSET_sBx);
      while (startpc<endpc) {
        fs->f->code[startpc] = CREATE_sJ(OP_JMP, (endpc-startpc-1) + OFFSET_sJ, 0);
        startpc++;
      }
      nret++;
    } else {
      luaK_codeABC(ls->fs, OP_CALL, first, nret + 1, 2);
      ls->fs->freereg = first + 1;
      nret = 1;
    }
  }
  luaK_ret(fs, first, nret);
  testnext(ls, ';');  /* skip optional semicolon */
}


static int checkkeyword (LexState *ls) {
  if (ls->t.token == TK_NAME) {
    for (int i = FIRST_NON_COMPAT; i != END_OPTIONAL; ++i) {
      if (strcmp(luaX_reserved2str(i), getstr(ls->t.seminfo.ts)) == 0) {
        luaX_next(ls);
        return i;
      }
    }
  }
  if (!ls->t.IsNonCompatible() && !ls->t.IsOptional()) {
    if (ls->t.IsCompatible())
      luaX_syntaxerror(ls, "expected non-compatible keyword");
    luaX_syntaxerror(ls, "expected keyword");
  }
  int token = ls->t.token;
  luaX_next(ls);
  return token;
}

static void enablekeyword (LexState *ls, int token) {
  const char* str = luaX_reserved2str(token);
  auto i = ls->tokens.begin() + ls->tidx;
  TString* ts = nullptr;
  /* find first instance of token */
  for (; i != ls->tokens.end(); ++i)
    if (i->token == TK_NAME && strcmp(str, getstr(i->seminfo.ts)) == 0) {
      ts = i->seminfo.ts;
      i->token = token;
      break;
    }
  /* find further instances of the token; faster now that we have TString */
  if (ts)
    for (; i != ls->tokens.end(); ++i)
      if (i->token == TK_NAME && eqstr(i->seminfo.ts, ts))
        i->token = token;
}

static void togglekeyword (LexState *ls, int token, bool enable) {
  if (ls->isKeywordEnabled(token) != enable) {
    ls->setKeywordState(token, enable ? KS_ENABLED_BY_SCRIPTER : KS_DISABLED_BY_SCRIPTER);
    if (enable)
      enablekeyword(ls, token);
    else
      disablekeyword(ls, token);
  }
}

static void usestat (LexState *ls) {
  const bool is_ann = ls->t.token == TK_USEANN;
  const auto line = ls->t.line;
  luaX_next(ls); /* skip 'pluto_use' */
  do {
    /* check affected tokens */
    bool is_all = false;
    bool is_version = false;
    std::vector<int> tokens{};
    if (ls->t.token == '*') {
      is_all = true;
      for (int i = FIRST_NON_COMPAT; i != END_OPTIONAL; ++i) {
        tokens.emplace_back(i);
      }
      luaX_next(ls);
    }
    else if (ls->t.token == TK_STRING) {
      is_version = true;
      if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.8.0") >= 0) {
        tokens = { TK_SWITCH, TK_CONTINUE, TK_ENUM, TK_NEW, TK_CLASS, TK_PARENT, TK_EXPORT, TK_TRY, TK_CATCH };
      }
      else if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.6.0") >= 0) {
        tokens = { TK_SWITCH, TK_CONTINUE, TK_ENUM, TK_NEW, TK_CLASS, TK_PARENT, TK_EXPORT };
      }
      else if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.5.0") >= 0) {
        tokens = { TK_SWITCH, TK_CONTINUE, TK_ENUM };
      }
      else if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.2.0") >= 0) {
        tokens = { TK_SWITCH, TK_CONTINUE };
      }
      else throwerr(ls, luaO_fmt(ls->L, "'pluto_use \"%s\"' is not valid", getstr(ls->t.seminfo.ts)), "did you mean \"0.8.0\", \"0.6.0\", \"0.5.0\" or \"0.2.0\"?");
      if (getstr(ls->t.seminfo.ts)[ls->t.seminfo.ts->size() - 1] == '+') {
        if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.9.0") >= 0) {
          tokens.emplace_back(TK_GLOBAL);
          /* 'let' and 'const' are deprecated as of 0.9.0, so we don't wanna enable them with `pluto_use "0.9.0+"` */
        }
        else if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.7.0") >= 0) {
          tokens.emplace_back(TK_LET);
          if (soup::version_compare(getstr(ls->t.seminfo.ts), "0.8.0") >= 0) {
            tokens.emplace_back(TK_CONST);
          }
        }
      }
      luaX_next(ls);
    }
    else {
      tokens = { checkkeyword(ls) };
    }

    /* check if enable or disable */
    bool enable = true;
    if (testnext(ls, '=')) {
      if (testnext(ls, TK_FALSE))
        enable = false;
      else checknext(ls, TK_TRUE);
    }

    /* catch stupid stuff */
    if (is_all && enable)
      throw_warn(ls, "'pluto_use *' is a bad idea because future Pluto versions may add keywords that will break your script", "consider using 'pluto_use \"0.8.0\"' instead", line, WT_BAD_PRACTICE);
    if (is_version && !enable)
      throwerr(ls, "'pluto_use <version>' is not valid for disabling tokens", "did you mean 'pluto_use * = false'?");

    /* apply change */
    if (is_all || is_version) {
      if (is_version) {
        /* disable all non-compatible keywords as of this Pluto version, then enable those from the elected Pluto version. */
        for (int i = FIRST_NON_COMPAT; i != END_OPTIONAL; ++i) {
          if (ls->isKeywordEnabled(i)) {
            ls->setKeywordState(i, KS_DISABLED_BY_SCRIPTER);
            disablekeyword(ls, i);
          }
        }
      }
      for (const auto& token : tokens) {
        togglekeyword(ls, token, enable);
      }
    }
    else {
      togglekeyword(ls, tokens.at(0), enable);
    }
  } while (testnext(ls, ','));

  /* update ls->t */
  luaX_setpos(ls, luaX_getpos(ls));

  if (is_ann) {
    /* skip remaining tokens in line */
    while (ls->t.line == line && ls->t.token != TK_EOS)
      luaX_next(ls);
  }
}

static void test_eqi (LexState *ls, expdesc* v, lua_Integer val, BinOpr op) {
  expdesc ival;
  luaK_infix(ls->fs, op, v);
  init_exp(&ival, VKINT, 0);
  ival.u.ival = val++;
  luaK_posfix(ls->fs, op, v, &ival, ls->getLineNumber());
}

static void ret_int(FuncState* fs, lua_Integer val) {
  expdesc v;
  init_exp(&v, VKINT, 0);
  v.u.ival = val;
  luaK_exp2anyreg(fs, &v);
  luaK_ret(fs, v.u.reg, 1);
}

static void trystat (LexState *ls) {
  BlockCnt trybl;
  expdesc ex;
  FuncState new_fs;
  BlockCnt bl;
  int exitjump = NO_JUMP;

  enterblock(ls->fs, &trybl, BlockType::BT_DEFAULT);

  ls->used_try = true;

  const auto line = ls->getLineNumber();
  luaX_next(ls);
  const bool vararg = ls->fs->f->is_vararg;

  /* temp (try status), (try result), (try result2), (try result3) = */
  const auto status_reg = ls->fs->freereg;
  const auto result_reg = ls->fs->freereg+1;

  /* temp (try status), (try result), (try result2), (try result3) = pcall */
  singlevar(ls, &ex, luaX_newliteral(ls, "pcall"));
  luaK_exp2nextreg(ls->fs, &ex);

  /* temp (try status), (try result), (try result2), (try result3) = pcall(function() */
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = ls->getLineNumber();
  open_func(ls, &new_fs, &bl);
  new_fs.istrybody = 1;
  if (vararg) {
    /* temp (try status), (try result), (try result2), (try result3) = pcall(function(...) */
    setvararg(&new_fs, 0);
  }

  /* temp (try status), (try result), (try result2), (try result3) = pcall(function(...) STATLIST */
  TypeHint prop{};
  statlist(ls, &prop);

  /* Move gotoes out of try/catch block */
  lua_assert(bl.firstgoto == trybl.firstgoto);
  Labellist *gl = &ls->dyd->gt;
  if (bl.firstgoto != gl->n) { /* There are some gotos that need to be moved out of the try/catch block */
    luaK_ret(&new_fs, luaY_nvarstack(&new_fs), 0);  /* Make sure the function body returns */
    removevars(&new_fs, bl.nactvar);  /* remove function locals */
    lua_assert(bl.nactvar == new_fs.nactvar);  /* no variables should remain */
    /* correct pending gotos to current block */
    int id = 3; // Next id of the goto return status
    for (int i = bl.firstgoto; i < gl->n; i++) {  /* for each pending goto */
      Labeldesc *gt = &gl->arr[i];
      int lab = luaK_getlabel(&new_fs);
      if (gt->pc != NO_JUMP) { /* Needs patching up */
        luaK_patchlist(&new_fs, gt->pc, lab);
        for (int j = i + 1; j < gl->n; j++) { /* Look for other gotos with the same name and patch them together */
          if (samelabelnames(&gl->arr[j], gt)) {
            luaK_patchlist(&new_fs, gl->arr[j].pc, lab);
            gl->arr[j].pc = NO_JUMP; /* This goto will also later be found, make sure it is later skipped but reamins in this list */
          }
        }
        ret_int(&new_fs, id++); /* Return the goto status */
      }
      gt->close = 0; /* Goto is moved out of the function, it is now new, so does for now not need closing. */
      gt->nactvar = bl.nactvar;  /* update goto level */
    }
    bl.firstgoto = gl->n; /* Prevent undefgoto errors here on the close_func */
  }

  /* temp (try status), (try result), (try result2), (try result3) = pcall(function(...) STATLIST end */
  new_fs.f->lastlinedefined = ls->getLineNumber();
  codeclosure(ls, &ex);
  close_func(ls);
  luaK_exp2nextreg(ls->fs, &ex);
  lua_assert(status_reg+1 == ex.u.reg);

  if (vararg) {
    /* temp (try status), (try result), (try result2), (try result3) = pcall(function(...) STATLIST end, ... */
    init_exp(&ex, VVARARG, luaK_codeABC(ls->fs, OP_VARARG, 0, 0, 1));
    luaK_setmultret(ls->fs, &ex);
  }

  /* temp (try status), (try result), (try result2), (try result3) = pcall(function(...) STATLIST end, ...) */
  init_exp(&ex, VCALL, luaK_codeABC(ls->fs, OP_CALL, status_reg, (vararg ? LUA_MULTRET : 1) + 1, 2));
  ls->fs->freereg = status_reg+1;

  int num_out = new_fs.seenrets & (1<<2) ? 4 : new_fs.seenrets & (1<<1) ? 3 : 2;
  adjust_assign(ls, num_out, 1, &ex);
  lua_assert(status_reg+num_out == ls->fs->freereg);

  if (!testnext(ls, TK_CATCH))
    check_match(ls, TK_PCATCH, TK_PTRY, line);

  auto old_pin = ls->fs->pinnedreg;

  /* if not (try status) then jump to catch block */
  ls->fs->pinnedreg = status_reg;
  init_exp(&ex, VNONRELOC, status_reg);

  if (trybl.firstgoto == gl->n && !new_fs.seenrets) {
    /* Simple case */
    /* Try body with only a falltrough case */
    /* Just jump over the catch block if there is no error */
    luaK_goiffalse(ls->fs, &ex);
    exitjump = ex.t;
  } else {
    /* Hard case */
    /* Try body had some special exits */
    /* Jump to catch if there was an error */
    luaK_goiftrue(ls->fs, &ex);
    int skip = ex.f;

    /* Jump to end of try/catch in case of falltrough */
    ls->fs->pinnedreg = result_reg;
    init_exp(&ex, VNONRELOC, result_reg);
    luaK_goiftrue(ls->fs, &ex);
    exitjump = ex.f;

    /* For every goto from the try body make a if case and update the jump */
    int id = 3;
    int j = trybl.firstgoto;
    for (int i = trybl.firstgoto; i < gl->n; i++) {
      Labeldesc *gt = &gl->arr[i];
      Labeldesc *lb = gt->special ? 0 : findlabel(ls, (TString*)gt->name);
      if (gt->pc != NO_JUMP) {
        init_exp(&ex, VNONRELOC, result_reg);
        test_eqi(ls, &ex, id++, OPR_EQ);

        if (lb) {  /* found a label */
          /* backward jump; will be resolved here */
          int lblevel = reglevel(ls->fs, lb->nactvar);  /* label level */
          if (luaY_nvarstack(ls->fs) > lblevel) { /* leaving the scope of a variable? */
            luaK_goiftrue(ls->fs, &ex);
            luaK_codeABC(ls->fs, OP_CLOSE, lblevel, 0, 0);
            /* create jump and link it to the label */
            luaK_patchlist(ls->fs, luaK_jump(ls->fs), lb->pc);
            luaK_patchtohere(ls->fs, ex.f);
          } else {
            luaK_goiffalse(ls->fs, &ex);
            luaK_patchlist(ls->fs, ex.t, lb->pc);
          }

        } else {
          /* Forward jumps need a new pc */
          luaK_goiffalse(ls->fs, &ex);
          gt->pc = ex.t;
        }
      }
      if (!lb) {
        /* If no label was found this goto needs to be retained, other ones removed */
        if (j != i)
          gl->arr[j] = gl->arr[i];
        j++;
      }
    }
    /* Update new number of pending gotos */
    gl->n = j;

    /* Update return status */
    ls->fs->seenrets |= new_fs.seenrets;

    if (ls->fs->istrybody) {
      /* We are ourselves in a try body */
      if (new_fs.seenrets) {
        /* Just return the result as is */
        luaK_ret(ls->fs, result_reg, num_out-1);
      }
    } else {
      for (int i=0; i<3; i++) {
        /* Add specialized returns for specific numbers */
        /* This is required for the correct number of returns */
        if (new_fs.seenrets & (1<<i)) {
          ex.f = NO_JUMP;
          if (new_fs.seenrets >> (i+1)) {
            init_exp(&ex, VNONRELOC, result_reg);
            test_eqi(ls, &ex, i, OPR_EQ);
            luaK_goiftrue(ls->fs, &ex);
          }
          luaK_ret(ls->fs, result_reg+1, i);
          luaK_patchtohere(ls->fs, ex.f);
        }
      }

      if (new_fs.seenrets & (1<<3)) {
        /* In the try body was a return with more then two returns */

        ls->fs->freereg = status_reg+2;

        /* table.unpack */
        expdesc key;
        singlevar(ls, &ex, luaX_newliteral(ls, "table"));
        luaK_exp2anyregup(ls->fs, &ex);
        codestring(&key, luaX_newliteral(ls, "unpack"));
        luaK_indexed(ls->fs, &ex, &key);
        luaK_exp2reg(ls->fs, &ex, status_reg);
        lua_assert(ex.k == VNONRELOC);

        /* results is already in result_reg == status_reg+1 */

        /* table.unpack(results, 1 */
        init_exp(&ex, VKINT, 0);
        ex.u.ival = 1;
        luaK_exp2nextreg(ls->fs, &ex);
        lua_assert(status_reg+2 == ex.u.reg);

        /* table.unpack(results, 1, results.n */
        init_exp(&ex, VNONRELOC, result_reg);
        codestring(&key, luaX_newliteral(ls, "n"));
        luaK_indexed(ls->fs, &ex, &key);
        luaK_exp2nextreg(ls->fs, &ex);
        lua_assert(status_reg+3 == ex.u.reg);

        /* table.unpack(results, 1, results.n) */
        luaK_codeABC(ls->fs, trybl.insidetbc ? OP_CALL : OP_TAILCALL, status_reg, 4 /* 3 params */, LUA_MULTRET + 1);
        luaK_ret(ls->fs, status_reg, LUA_MULTRET);
      }
    }

    luaK_patchtohere(ls->fs, skip);
  }

  ls->fs->freereg = status_reg+2;
  ls->fs->pinnedreg = old_pin;

  /* local (e) */
  new_localvar(ls, str_checkname(ls, N_OVERRIDABLE));

  const auto then_line = ls->getLineNumber();
  if (!testnext(ls, TK_DO))
    checknext(ls, TK_THEN);

  /* local (e) = (try result) */
  init_exp(&ex, VNONRELOC, result_reg);
  luaK_exp2reg(ls->fs, &ex, status_reg);
  lua_assert(status_reg == luaY_nvarstack(ls->fs));
  lua_assert(status_reg+1 == ls->fs->freereg);
  adjustlocalvars(ls, 1);

  enterblock(ls->fs, &bl, BlockType::BT_DEFAULT);
  bl.nactvar--; /* Move e into this block */

  statlist(ls);

  /* end */
  check_match(ls, TK_END, TK_PCATCH, then_line);
  leaveblock(ls->fs);

  luaK_patchtohere(ls->fs, exitjump);
  leaveblock(ls->fs);
}


static void globalstat (LexState *ls) {
  const auto line = ls->getLineNumber();
  luaX_next(ls);  /* skip GLOBAL */
  if (ls->t.token == TK_FUNCTION) {
    luaX_next(ls);  /* skip FUNCTION */
    ls->explicit_globals.emplace(str_checkname(ls));
    check(ls, '(');
    luaX_prev(ls);
    funcstat(ls, line, true);
  }
  else if (ls->t.token == TK_CLASS) {
    luaX_next(ls);  /* skip CLASS */
    ls->explicit_globals.emplace(str_checkname(ls));
    luaX_prev(ls);
    classstat(ls, line, true);
  }
  else {
    size_t tidx = luaX_getpos(ls);
    do {
      ls->explicit_globals.emplace(str_checkname(ls));
    } while (testnext(ls, ','));
    if (ls->t.token == '=') {
      luaX_setpos(ls, tidx);
      struct LHS_assign v;
      primaryexp(ls, &v.v);
      if (ls->t.token != ',')
        check(ls, '=');
      v.prev = NULL;
      restassign(ls, &v, 1);
    }
  }
}


static void statement (LexState *ls, TypeHint *prop) {
#ifdef PLUTO_PARSER_SUGGESTIONS
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", "if");
    ss.push("stat", "while");
    ss.push("stat", "do");
    ss.push("stat", "for");
    ss.push("stat", "function");
    ss.push("stat", "class");
    ss.push("stat", "pluto_class");
    ss.push("stat", "local");
    ss.push("stat", "export");
    ss.push("stat", "pluto_export");
    ss.push("stat", "return");
    ss.push("stat", "break");
    ss.push("stat", "continue");
    ss.push("stat", "goto");
    ss.push("stat", "switch");
    ss.push("stat", "pluto_switch");
    ss.push("stat", "enum");
    ss.push("stat", "pluto_use");
    ss.push("stat", "new");
    ss.push("stat", "pluto_new");
    ss.push("stat", "try");
    ss.push("stat", "pluto_try");
    if (ls->fs->bl->previous)
      ss.push("stat", "end");
    ss.pushLocals();
    return;
  }
#endif
  int line = ls->getLineNumber();
  if (ls->t.token != ';') {
    if (ls->laststat.IsEscapingToken()
      || (ls->laststat.Is(TK_GOTO) && ls->t.token != TK_DBCOLON)) /* Don't warn if this statement is a goto label. */
    {
      throw_warn(ls,
        "unreachable code",
          luaO_fmt(ls->L, "this code comes after an escaping %s statement.", luaX_token2str(ls, ls->laststat.token)), WT_UNREACHABLE_CODE);
      ls->L->top.p--;
    }
    ls->laststat.token = ls->t.token;
  }
  enterlevel(ls);
  check_for_non_portable_code(ls);
  switch (ls->t.token) {
    case ';': {  /* stat -> ';' (empty statement) */
      luaX_next(ls);  /* skip ';' */
      break;
    }
    case TK_IF: {  /* stat -> ifstat */
      ifstat(ls, line, prop);
      break;
    }
    case '$': {
      luaX_next(ls);
      constexprstat(ls, line);
      break;
    }
    case TK_WHILE: {  /* stat -> whilestat */
      whilestat(ls, line);
      break;
    }
    case TK_DO: {  /* stat -> DO block END */
      luaX_next(ls);  /* skip DO */
      block(ls);
      check_match(ls, TK_END, TK_DO, line);
      break;
    }
    case TK_FOR: {  /* stat -> forstat */
      forstat(ls, line, prop);
      break;
    }
    case TK_REPEAT: {  /* stat -> repeatstat */
      repeatstat(ls);
      break;
    }
    case TK_FUNCTION: {  /* stat -> funcstat */
      luaX_next(ls);  /* skip FUNCTION */
      funcstat(ls, line, false);
      break;
    }
    case TK_CLASS:
    case TK_PCLASS: {
      luaX_next(ls);  /* skip CLASS */
      classstat(ls, line, false);
      break;
    }
    case TK_LET:
      throw_warn(ls, "'let' will be removed in future versions of Pluto. use 'local' instead.", WT_DEPRECATED);
      [[fallthrough]];
    case TK_LOCAL: {  /* stat -> localstat */
      luaX_next(ls);  /* skip LOCAL */
#ifdef PLUTO_PARSER_SUGGESTIONS
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("stat", "function");
        ss.push("stat", "class");
        ss.push("stat", "pluto_class");
        return;
      }
#endif
      if (testnext(ls, TK_FUNCTION))  /* local function? */
        localfunc(ls);
      else if (testnext2(ls, TK_CLASS, TK_PCLASS)) {
        if (ls->t.token == '=') {
          if (luaX_lookbehind(ls).token == TK_CLASS && ls->getKeywordState(TK_CLASS) == KS_ENABLED_BY_PLUTO_UNINFORMED) {
            luaX_prev(ls);
            disablekeyword(ls, TK_CLASS);
            ls->uninformed_reserved.emplace(TK_CLASS, ls->getLineNumber());
            ls->setKeywordState(TK_CLASS, KS_DISABLED_BY_PLUTO_INFORMED);
            luaX_setpos(ls, luaX_getpos(ls));  /* update ls->t */
            localstat(ls);
          }
          else {
            throwerr(ls, "expected a class name, found '='", "'class' has a different meaning in Pluto, but you can disable this: https://pluto.do/compat");
          }
        }
        else {
          localclass(ls);
        }
      }
      else
        localstat(ls);
      break;
    }
    case TK_CONST: {
      throw_warn(ls, "'const' will be removed in future versions of Pluto. use 'local' instead.", WT_DEPRECATED);
      luaX_next(ls);  /* skip CONST */
      conststat(ls);
      break;
    }
    case TK_EXPORT:
    case TK_PEXPORT: {
      luaX_next(ls);  /* skip EXPORT */
#ifdef PLUTO_PARSER_SUGGESTIONS
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("stat", "function");
        ss.push("stat", "class");
        ss.push("stat", "pluto_class");
        break;
      }
#endif
      throw_warn(ls, "'export' is deprecated. use a table instead.", WT_DEPRECATED);
      if (testnext(ls, TK_FUNCTION)) {
        ls->fs->bl->export_symbols.emplace_back(str_checkname(ls, 0));
        luaX_prev(ls);
        localfunc(ls);
      }
      else if (testnext2(ls, TK_CLASS, TK_PCLASS)) {
        ls->fs->bl->export_symbols.emplace_back(str_checkname(ls, 0));
        luaX_prev(ls);
        localclass(ls);
      }
      else {
        ls->fs->bl->export_symbols.emplace_back(str_checkname(ls, 0));
        luaX_prev(ls);
        localstat(ls);
      }
      break;
    }
    case TK_DBCOLON: {  /* stat -> label */
      luaX_next(ls);  /* skip double colon */
      labelstat(ls, str_checkname(ls, N_RESERVED), line);
      break;
    }
    case TK_RETURN: {  /* stat -> retstat */
      luaX_next(ls);  /* skip RETURN */
      retstat(ls, prop);
      break;
    }
    case TK_BREAK: {  /* stat -> breakstat */
      breakstat(ls);
      break;
    }
    case TK_CONTINUE:
    case TK_PCONTINUE: {
      continuestat(ls);
      break;
    }
    case TK_GOTO: {  /* stat -> 'goto' NAME */
      luaX_next(ls);  /* skip 'goto' */
      gotostat(ls);
      break;
    }
    case TK_SWITCH:
    case TK_PSWITCH: {
      switchstat(ls);
      break;
    }
    case TK_ENUM:
    case TK_PENUM: {
      enumstat(ls);
      break;
    }
    case TK_PUSE:
    case TK_USEANN: {
      usestat(ls);
      break;
    }
    case '+': {
      luaX_next(ls);
      check(ls, '+');
      expdesc v;
      prefixplusplus(ls, &v, true);
      break;
    }
    case TK_NEW:
    case TK_PNEW: {
      expdesc v;
      newexpr(ls, &v);
      expsuffix(ls, &v, line, 0, prop);
      break;
    }
    case TK_TRY:
    case TK_PTRY: {
      trystat(ls);
      break;
    }
    case TK_GLOBAL: {
      globalstat(ls);
      break;
    }
    default: {  /* stat -> func | assignment */
      exprstat(ls);
      break;
    }
  }
  lua_assert(ls->fs->f->maxstacksize >= ls->fs->freereg &&
             ls->fs->freereg >= luaY_nvarstack(ls->fs));
  ls->fs->freereg = luaY_nvarstack(ls->fs);  /* free registers */
  leavelevel(ls);
}

/* }====================================================================== */


static void builtinoperators (LexState *ls) {
  if (ls->uses_new || ls->uses_extends || ls->uses_instanceof || ls->uses_spaceship) {
    /* capture state */
    std::vector<Token> tokens = std::move(ls->tokens);

    ls->tokens = {}; /* avoid use of moved warning */

    if (ls->uses_new) {
      // local Pluto_operator_new <const> = function(mt, ...)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "Pluto_operator_new")));
#ifndef PLUTO_ALLOW_FUNCTION_INJECTION_REASSIGNMENT
      ls->tokens.emplace_back(Token('<'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "const")));
      ls->tokens.emplace_back(Token('>'));
#endif
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_DOTS));
      ls->tokens.emplace_back(Token(')'));

      //   if mt.new then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "new")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     return mt.new(...)
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "new")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_DOTS));
      ls->tokens.emplace_back(Token(')'));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   local t = {}
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token('{'));
      ls->tokens.emplace_back(Token('}'));

      //   setmetatable(t, mt)
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "setmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token(')'));

      //   if not rawget(mt, "__index") then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NOT));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "rawget")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__index")));
      ls->tokens.emplace_back(Token(')'));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     mt.__index = mt
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__index")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   if mt.__construct then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__construct")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     mt.__construct(t, ...)
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__construct")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_DOTS));
      ls->tokens.emplace_back(Token(')'));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   return t
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }
    if (ls->uses_extends) {
      // local Pluto_operator_extends <const> = function(c, p)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "Pluto_operator_extends")));
#ifndef PLUTO_ALLOW_FUNCTION_INJECTION_REASSIGNMENT
      ls->tokens.emplace_back(Token('<'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "const")));
      ls->tokens.emplace_back(Token('>'));
#endif
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "c")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));
      ls->tokens.emplace_back(Token(')'));

      // if (p_type := type(p)) != "table" then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p_type")));
      ls->tokens.emplace_back(Token(TK_WALRUS));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "type")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));
      ls->tokens.emplace_back(Token(')'));
      ls->tokens.emplace_back(Token(')'));
      ls->tokens.emplace_back(Token(TK_NE));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "table")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //   error("expected a class or class-like table to extend, got " .. p_type)
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "error")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "expected a class or class-like table to extend, got ")));
      ls->tokens.emplace_back(Token(TK_CONCAT));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p_type")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_INT, 2));
      ls->tokens.emplace_back(Token(')'));

      // end
      ls->tokens.emplace_back(Token(TK_END));

      //   for { ... } as mm do
      ls->tokens.emplace_back(Token(TK_FOR));
      ls->tokens.emplace_back(Token('{'));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__gc")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__mode")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__len")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__eq")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__add")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__sub")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__mul")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__mod")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__pow")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__div")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__idiv")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__band")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__bor")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__bxor")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__shl")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__shr")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__unm")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__bnot")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__lt")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__le")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__concat")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__call")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__close")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_STRING, luaX_newliteral(ls, "__tostring")));
      ls->tokens.emplace_back(Token('}'));
      ls->tokens.emplace_back(Token(TK_AS));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mm")));
      ls->tokens.emplace_back(Token(TK_DO));

      //     if p[mm] and not c[mm] then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));
      ls->tokens.emplace_back(Token('['));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mm")));
      ls->tokens.emplace_back(Token(']'));
      ls->tokens.emplace_back(Token(TK_AND));
      ls->tokens.emplace_back(Token(TK_NOT));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "c")));
      ls->tokens.emplace_back(Token('['));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mm")));
      ls->tokens.emplace_back(Token(']'));
      ls->tokens.emplace_back(Token(TK_THEN));

      //      c[mm] = p[mm]
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "c")));
      ls->tokens.emplace_back(Token('['));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mm")));
      ls->tokens.emplace_back(Token(']'));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));
      ls->tokens.emplace_back(Token('['));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mm")));
      ls->tokens.emplace_back(Token(']'));

      //     end
      ls->tokens.emplace_back(Token(TK_END));
      
      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   setmetatable(c, { __index = p })
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "setmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "c")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token('{'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__index")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));
      ls->tokens.emplace_back(Token('}'));
      ls->tokens.emplace_back(Token(')'));

      //   c.__parent = p
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "c")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__parent")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "p")));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }
    if (ls->uses_instanceof) {
      // local Pluto_operator_instanceof <const> = function(t, mt)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "Pluto_operator_instanceof")));
#ifndef PLUTO_ALLOW_FUNCTION_INJECTION_REASSIGNMENT
      ls->tokens.emplace_back(Token('<'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "const")));
      ls->tokens.emplace_back(Token('>'));
#endif
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token(')'));

      //   t = getmetatable(t)
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "getmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(')'));

      //   while t do
      ls->tokens.emplace_back(Token(TK_WHILE));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(TK_DO));

      //     if t == mt then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token(TK_EQ));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "mt")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //       return true
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_TRUE));

      //     end
      ls->tokens.emplace_back(Token(TK_END));

      //     t = t.__parent
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "t")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "__parent")));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   return false
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_FALSE));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }
    if (ls->uses_spaceship) {
      // local Pluto_operator_spaceship <const> = function(a, b)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "Pluto_operator_spaceship")));
#ifndef PLUTO_ALLOW_FUNCTION_INJECTION_REASSIGNMENT
      ls->tokens.emplace_back(Token('<'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "const")));
      ls->tokens.emplace_back(Token('>'));
#endif
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "a")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "b")));
      ls->tokens.emplace_back(Token(')'));

      //   return a ~= b ? a < b ? -1 : +1 : 0
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "a")));
      ls->tokens.emplace_back(Token(TK_NE));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "b")));
      ls->tokens.emplace_back(Token('?'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "a")));
      ls->tokens.emplace_back(Token('<'));
      ls->tokens.emplace_back(Token(TK_NAME, luaX_newliteral(ls, "b")));
      ls->tokens.emplace_back(Token('?'));
      ls->tokens.emplace_back(Token(TK_INT, (lua_Integer)-1));
      ls->tokens.emplace_back(Token(':'));
      ls->tokens.emplace_back(Token(TK_INT, (lua_Integer)+1));
      ls->tokens.emplace_back(Token(':'));
      ls->tokens.emplace_back(Token(TK_INT, (lua_Integer)0));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }

    /* parse */
    ls->tokens.emplace_back(Token(TK_EOS));
    luaX_next(ls);
    statlist(ls);

    /* reset state */
    ls->tidx = -1;
    ls->tokens = std::move(tokens);
  }
}


/*
** compiles the main function, which is a regular vararg function with an
** upvalue named LUA_ENV
*/
static void mainfunc (LexState *ls, FuncState *fs) {
  BlockCnt bl;
  Upvaldesc *env;
  open_func(ls, fs, &bl);
  setvararg(fs, 0);  /* main function is always declared vararg */
  env = allocupvalue(fs);  /* ...set environment upvalue */
  env->instack = 1;
  env->idx = 0;
  env->kind = VDKREG;
  env->name = ls->envn;
  luaC_objbarrier(ls->L, fs->f, env->name);
  builtinoperators(ls);
  luaX_next(ls);  /* read first token */
  statlist(ls);  /* parse main body */
  check(ls, TK_EOS);
  close_func(ls);
}


static void applyenvkeywordpreference (LexState *ls, int t, bool b) {
  if (b) {
    ls->setKeywordState(t, KS_ENABLED_BY_ENV);
  }
  else {
    disablekeyword(ls, t);
    ls->setKeywordState(t, KS_DISABLED_BY_ENV);
  }
}


LClosure *luaY_parser (lua_State *L, LexState& lexstate, ZIO *z, Mbuffer *buff,
                       Dyndata *dyd, const char *name, int firstchar) {
  FuncState funcstate;
  LClosure *cl = luaF_newLclosure(L, 1);  /* create main closure */
  setclLvalue2s(L, L->top.p, cl);  /* anchor it (to avoid being collected) */
  luaD_inctop(L);
  lexstate.h = luaH_new(L);  /* create table for scanner */
  sethvalue2s(L, L->top.p, lexstate.h);  /* anchor it */
  luaD_inctop(L);
  funcstate.f = cl->p = luaF_newproto(L);
  luaC_objbarrier(L, cl, cl->p);
  funcstate.f->source = luaS_new(L, name);  /* create and anchor TString */
  luaC_objbarrier(L, funcstate.f, funcstate.f->source);
  lexstate.buff = buff;
  lexstate.dyd = dyd;
  dyd->actvar.n = dyd->gt.n = dyd->label.n = 0;
  luaX_setinput(L, &lexstate, z, funcstate.f->source, firstchar);
  if (L->l_G->have_preference_switch)
    applyenvkeywordpreference(&lexstate, TK_SWITCH, L->l_G->preference_switch);
  if (L->l_G->have_preference_continue)
    applyenvkeywordpreference(&lexstate, TK_CONTINUE, L->l_G->preference_continue);
  if (L->l_G->have_preference_enum)
    applyenvkeywordpreference(&lexstate, TK_ENUM, L->l_G->preference_enum);
  if (L->l_G->have_preference_new)
    applyenvkeywordpreference(&lexstate, TK_NEW, L->l_G->preference_new);
  if (L->l_G->have_preference_class)
    applyenvkeywordpreference(&lexstate, TK_CLASS, L->l_G->preference_class);
  if (L->l_G->have_preference_parent)
    applyenvkeywordpreference(&lexstate, TK_PARENT, L->l_G->preference_parent);
  if (L->l_G->have_preference_export)
    applyenvkeywordpreference(&lexstate, TK_EXPORT, L->l_G->preference_export);
  if (L->l_G->have_preference_try)
    applyenvkeywordpreference(&lexstate, TK_TRY, L->l_G->preference_try);
  if (L->l_G->have_preference_catch)
    applyenvkeywordpreference(&lexstate, TK_CATCH, L->l_G->preference_catch);
#ifndef PLUTO_USE_LET
  disablekeyword(&lexstate, TK_LET);
#endif
#ifndef PLUTO_USE_CONST
  disablekeyword(&lexstate, TK_CONST);
#endif
#ifndef PLUTO_USE_GLOBAL
  disablekeyword(&lexstate, TK_GLOBAL);
#endif
  mainfunc(&lexstate, &funcstate);
  lua_assert(!funcstate.prev && funcstate.nups == 1 && !lexstate.fs);
  /* all scopes should be correctly finished */
  lua_assert(dyd->actvar.n == 0 && dyd->gt.n == 0 && dyd->label.n == 0);
  L->top.p--;  /* remove scanner's table */
  return cl;  /* closure is on the stack, too */
}
