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
#include "lauxlib.h"

#include "lerrormessage.hpp"



#define hasmultret(k)		((k) == VCALL || (k) == VVARARG)


/*
** Invokes the lua_writestring macro with a std::string.
*/
#define write_std_string(std_string) lua_writestring(std_string.data(), std_string.size())


/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define eqstr(a,b)	((a) == (b))


#define luaO_fmt luaO_pushfstring


/*
** nodes for block list (list of active blocks)
*/
typedef struct BlockCnt {
  struct BlockCnt *previous;  /* chain */
  int breaklist;  /* list of jumps out of this loop */
  int scopeend;  /* delimits the end of this scope, for 'continue' to jump before. */
  int firstlabel;  /* index of first label in this block */
  int firstgoto;  /* index of first pending goto in this block */
  int nactvar;  /* # active locals outside the block */
  lu_byte upval;  /* true if some variable in the block is an upvalue */
  lu_byte isloop;  /* true if 'block' is a loop */
  lu_byte insidetbc;  /* true if inside the scope of a to-be-closed var. */
} BlockCnt;



/*
** prototypes for recursive non-terminal functions
*/
static void statement (LexState *ls, TypeDesc *prop = nullptr);
static void expr (LexState *ls, expdesc *v, TypeDesc *prop = nullptr, int flags = 0);


/*
** Throws an exception into Lua, which will promptly close the program.
** This is only called for vital errors, like lexer and/or syntax problems.
*/
[[noreturn]] static void throwerr (LexState *ls, const char *err, const char *here, int line) {
  err = luaG_addinfo(ls->L, err, ls->source, line);
  Pluto::ErrorMessage msg{ ls, HRED "syntax error: " BWHT }; // We'll only throw syntax errors if 'throwerr' is called
  msg.addMsg(err)
    .addSrcLine(line)
    .addGenericHere(here)
    .finalizeAndThrow();
}

[[noreturn]] static void throwerr (LexState *ls, const char *err, const char *here) {
  throwerr(ls, err, here, ls->getLineNumber());
}


// No note.
static void throw_warn (LexState *ls, const char *raw_err, const char *here, int line, WarningType warningType) {
  std::string err(raw_err);
  if (ls->shouldEmitWarning(line, warningType)) {
    Pluto::ErrorMessage msg{ ls, luaG_addinfo(ls->L, YEL "warning: " BWHT, ls->source, line) };
    err.append(" [");
    err.append(ls->getWarningConfig().getWarningName(warningType));
    err.push_back(']');
    msg.addMsg(err)
      .addSrcLine(line)
      .addGenericHere(here)
      .finalize();
    lua_warning(ls->L, msg.content.c_str(), 0);
    ls->L->top.p -= 2; // Pluto::Error::finalize & luaG_addinfo
  }
}

// Note.
static void throw_warn(LexState* ls, const char* raw_err, const char* here, const char* note, int line, WarningType warningType) {
  std::string err(raw_err);
  if (ls->shouldEmitWarning(line, warningType)) {
    Pluto::ErrorMessage msg{ ls, luaG_addinfo(ls->L, YEL "warning: " BWHT, ls->source, line) };
    err.append(" [");
    err.append(ls->getWarningConfig().getWarningName(warningType));
    err.push_back(']');
    msg.addMsg(err)
      .addSrcLine(line)
      .addGenericHere(here)
      .addNote(note)
      .finalize();
    lua_warning(ls->L, msg.content.c_str(), 0);
    ls->L->top.p -= 2; // Pluto::Error::finalize & luaG_addinfo
  }
}

static void throw_warn(LexState *ls, const char* err, const char *here, WarningType warningType) {
  return throw_warn(ls, err, here, ls->getLineNumber(), warningType);
}

static void throw_warn(LexState *ls, const char *err, int line, WarningType warningType) {
  if (ls->shouldEmitWarning(line, warningType)) {
    std::string msg = luaG_addinfo(ls->L, YEL "warning: " BWHT, ls->source, line);
    msg.append(err);
    msg.append(" [");
    msg.append(ls->getWarningConfig().getWarningName(warningType));
    msg.push_back(']');
    lua_warning(ls->L, msg.c_str(), 0);
    ls->L->top.p -= 1; /* remove warning from stack */
  }
}

#pragma warning(disable : 4068) // unknown pragma
#pragma clang diagnostic ignored "-Wunused-function"
static void throw_warn(LexState* ls, const char* err, WarningType warningType) {
  throw_warn(ls, err, ls->getLineNumber(), warningType);
}


/*
** This function will throw an exception and terminate the program.
*/
[[noreturn]] static void error_expected (LexState *ls, int token) {
  switch (token) {
    case '|': {
      throwerr(ls,
        "expected '|' to control parameters.",
        "expected '|' to begin & terminate the lambda's paramater list.");
    }
    case '-': {
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
    case TK_THEN: {
      throwerr(ls,
        "expected 'then' to delimit condition.", "expected 'then' symbol.");
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
      throwerr(ls,
        luaO_fmt(ls->L, "%s expected (got %s)",
          luaX_token2str(ls, token), luaX_token2str(ls, ls->t.token)), "this is invalid syntax.");
    }
  }
}


[[noreturn]] static void errorlimit (FuncState *fs, int limit, const char *what) {
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


/*
** Check that next token is 'c'.
*/
static void check (LexState *ls, int c) {
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", luaX_token2str_noq(ls, c));
  }
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
static void check_match (LexState *ls, int what, int who, int where) {
  if (l_unlikely(!testnext(ls, what))) {
    if (where == ls->getLineNumber())  /* all in the same line? */
      error_expected(ls, what);  /* do not need a complex message */
    else {
      if (what == TK_END) {
        std::string msg = "missing 'end' to terminate ";
        msg.append(luaX_token2str(ls, who));
        if (who != TK_BEGIN) {
          msg.append(" block");
        }
        msg.append(" on line ");
        msg.append(std::to_string(where));
        throwerr(ls, msg.c_str(), "this was the last statement.", ls->getLineNumberOfLastNonEmptyLine());
      }
      else {
        Pluto::ErrorMessage err{ ls, RED "syntax error: " BWHT }; // Doesn't use throwerr since I replicated old code. Couldn't find problematic code to repro error, so went safe.
        err.addMsg(luaX_token2str(ls, what))
          .addMsg(" expected (to close ")
          .addMsg(luaX_token2str(ls, who))
          .addMsg(" on line ")
          .addMsg(std::to_string(where))
          .addMsg(")")
          .addSrcLine(ls->getLineNumberOfLastNonEmptyLine())
          .addGenericHere()
          .finalizeAndThrow();
      }
    }
  }
}


[[nodiscard]] static bool isnametkn(LexState *ls, bool strict = false) {
  return ls->t.token == TK_NAME || ls->t.IsNarrow() || (!strict && ls->t.IsReservedNonValue());
}


static TString *str_checkname (LexState *ls, bool strict = false) {
  TString *ts;
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.pushLocals();
  }
  if (!isnametkn(ls, strict)) {
    error_expected(ls, TK_NAME);
  }
  ts = ls->t.seminfo.ts;
  luaX_next(ls);
  return ts;
}


static void init_exp (expdesc *e, expkind k, int i) {
  e->f = e->t = NO_JUMP;
  e->k = k;
  e->u.info = i;
}


static void codestring (expdesc *e, TString *s) {
  e->f = e->t = NO_JUMP;
  e->k = VKSTR;
  e->u.strval = s;
}


static void codename (LexState *ls, expdesc *e) {
  codestring(e, str_checkname(ls));
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


[[nodiscard]] static TypeDesc gettypehint (LexState *ls, bool funcret = false) noexcept {
  /* TYPEHINT -> [':' Typedesc] */
  if (testnext(ls, ':')) {
    const bool nullable = testnext(ls, '?');
    const char* tname = getstr(str_checkname(ls));
    if (strcmp(tname, "number") == 0)
      return { VT_NUMBER, nullable };
    else if (strcmp(tname, "int") == 0)
      return { VT_INT, nullable };
    else if (strcmp(tname, "float") == 0)
      return { VT_FLT, nullable };
    else if (strcmp(tname, "table") == 0)
      return { VT_TABLE, nullable };
    else if (strcmp(tname, "string") == 0)
      return { VT_STR, nullable };
    else if (strcmp(tname, "boolean") == 0 || strcmp(tname, "bool") == 0)
      return { VT_BOOL, nullable };
    else if (strcmp(tname, "function") == 0)
      return { VT_FUNC, nullable };
    else if (strcmp(tname, "void") == 0) {
      if (!nullable && funcret)
        return { VT_VOID, nullable };
      throw_warn(ls, "'void' is not a valid type hint in this context", "invalid type hint", WT_TYPE_MISMATCH);
    }
    else if (strcmp(tname, "userdata") != 0) {
      luaX_prev(ls);
      throw_warn(ls, luaO_fmt(ls->L, "'%s' is not a type known to the parser", tname), "unknown type hint", WT_TYPE_MISMATCH);
      ls->L->top.p--;
      luaX_next(ls); // Preserve a6c8e359857644f4311c022f85cf19d85d95c25d
    }
  }
  return VT_DUNNO;
}


static void exp_propagate(LexState* ls, const expdesc& e, TypeDesc& t) noexcept {
  if (e.k == VLOCAL) {
    t = getlocalvardesc(ls->fs, e.u.var.vidx)->vd.prop;
  }
  else if (e.k == VCONST) {
    TValue* val = &ls->dyd->actvar.arr[e.u.info].k;
    switch (ttype(val))
    {
    case LUA_TNIL: t = VT_NIL; break;
    case LUA_TBOOLEAN: t = VT_BOOL; break;
    case LUA_TNUMBER: t = ((ttypetag(val) == LUA_VNUMINT) ? VT_INT : VT_FLT); break;
    case LUA_TSTRING: t = VT_STR; break;
    case LUA_TTABLE: t = VT_TABLE; break;
    case LUA_TFUNCTION: t = VT_FUNC; break;
    }
  }
}


static void process_assign(LexState* ls, Vardesc* var, const TypeDesc& td, int line) {
  auto hinted = var->vd.hint.getType() != VT_DUNNO;
  auto knownvalue = td.getType() != VT_DUNNO;
  auto incompatible = !var->vd.hint.isCompatibleWith(td);
  if (hinted && knownvalue && incompatible) {
    const auto hint = var->vd.hint.toString();
    std::string err = var->vd.name->toCpp();
    err.insert(0, "'");
    err.append("' type-hinted as '" + hint);
    err.append("', but assigned a ");
    err.append(td.toString());
    err.append(" value.");
    if (td.getType() == VT_NIL) {  /* Specialize warnings for nullable state incompatibility. */
      throw_warn(ls, "variable type mismatch", err.c_str(), luaO_fmt(ls->L, "try a nilable type hint: '?%s'", hint.c_str()), line, WT_TYPE_MISMATCH);
      ls->L->top.p--; // luaO_fmt
    }
    else {  /* Throw a generic mismatch warning. */
      throw_warn(ls, "variable type mismatch", err.c_str(), line, WT_TYPE_MISMATCH);
    }
  }
  var->vd.prop = td; /* propagate type */
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
** Create a new local variable with the given 'name'. Return its index
** in the function.
*/
static int new_localvar (LexState *ls, TString *name, int line, const TypeDesc& hint = VT_DUNNO) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Dyndata *dyd = ls->dyd;
  Vardesc *var;
  int locals = luaY_nvarstack(fs);
  for (int i = fs->firstlocal; i < locals; i++) {
    Vardesc *desc = getlocalvardesc(fs, i);
    LocVar *local = localdebuginfo(fs, i);
    std::string n = name->toCpp();
    if ((n != "(for state)" && n != "(switch control value)") && (local && local->varname == name)) { // Got a match.
      throw_warn(ls,
        "duplicate local declaration",
          luaO_fmt(L, "this shadows the initial declaration of '%s' on line %d.", name->contents, desc->vd.line), line, WT_VAR_SHADOW);
      L->top.p--; /* pop result of luaO_fmt */
      break;
    }
  }
  luaM_growvector(L, dyd->actvar.arr, dyd->actvar.n + 1,
                  dyd->actvar.size, Vardesc, USHRT_MAX, "local variables");
  var = &dyd->actvar.arr[dyd->actvar.n++];
  var->vd.kind = VDKREG;  /* default */
  var->vd.hint = hint;
  var->vd.prop = VT_DUNNO;
  var->vd.name = name;
  var->vd.line = line;
  return dyd->actvar.n - 1 - fs->firstlocal;
}

static int new_localvar (LexState *ls, TString *name, const TypeDesc& hint = VT_DUNNO) {
  return new_localvar(ls, name, ls->getLineNumber(), hint);
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
  if (hasmultret(e->k)) {  /* last expression has multiple returns? */
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
static void singlevarinner (LexState *ls, TString *varname, expdesc *var) {
  FuncState *fs = ls->fs;
  singlevaraux(fs, varname, var, 1);
  if (var->k == VVOID) {  /* global name? */
    expdesc key;
    singlevaraux(fs, ls->envn, var, 1);  /* get environment variable */
    lua_assert(var->k != VVOID);  /* this one must exist */
    luaK_exp2anyregup(fs, var);  /* but could be a constant */
    codestring(&key, varname);  /* key is variable name */
    luaK_indexed(fs, var, &key);  /* env[varname] */
  }
}

static void singlevar (LexState *ls, expdesc *var, TString *varname) {
  if (gett(ls) == TK_WALRUS) {
    luaX_next(ls);
    if (ls->getContext() == PARCTX_CREATE_VARS)
      throwerr(ls, "unexpected ':=' while creating multiple variable", "unexpected ':='");
    if (ls->getContext() == PARCTX_FUNCARGS)
      throwerr(ls, "unexpected ':=' while processing function arguments", "unexpected ':='");
    new_localvar(ls, varname);
    expr(ls, var);
    adjust_assign(ls, 1, 1, var);
    adjustlocalvars(ls, 1);
    return;
  }
  singlevarinner(ls, varname, var);
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
[[noreturn]] static void jumpscopeerror (LexState *ls, Labeldesc *gt) {
  const char *varname = getstr(getlocalvardesc(ls->fs, gt->nactvar)->vd.name);
  const char *msg = "<goto %s> at line %d jumps into the scope of local '%s'";
  msg = luaO_pushfstring(ls->L, msg, getstr(gt->name), gt->line, varname);
  luaK_semerror(ls, msg);  /* raise the error */
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
  lua_assert(eqstr(gt->name, label->name));
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
    if (eqstr(lb->name, name))  /* correct label? */
      return lb;
  }
  return NULL;  /* label not found */
}


/*
** Adds a new label/goto in the corresponding list.
*/
static int newlabelentry (LexState *ls, Labellist *l, TString *name,
                          int line, int pc) {
  int n = l->n;
  luaM_growvector(ls->L, l->arr, n, l->size,
                  Labeldesc, SHRT_MAX, "labels/gotos");
  l->arr[n].name = name;
  l->arr[n].line = line;
  l->arr[n].nactvar = ls->fs->nactvar;
  l->arr[n].close = 0;
  l->arr[n].pc = pc;
  l->n = n + 1;
  return n;
}


static int newgotoentry (LexState *ls, TString *name, int line, int pc) {
  return newlabelentry(ls, &ls->dyd->gt, name, line, pc);
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
    if (eqstr(gl->arr[i].name, lb->name)) {
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
static int createlabel (LexState *ls, TString *name, int line,
                        int last) {
  FuncState *fs = ls->fs;
  Labellist *ll = &ls->dyd->label;
  int l = newlabelentry(ls, ll, name, line, luaK_getlabel(fs));
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


static void enterblock (FuncState *fs, BlockCnt *bl, lu_byte isloop) {
  bl->breaklist = NO_JUMP;
  bl->isloop = isloop;
  bl->scopeend = NO_JUMP;
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
[[noreturn]] static void undefgoto (LexState *ls, Labeldesc *gt) {
  const char *msg;
  if (eqstr(gt->name, luaS_newliteral(ls->L, "break"))) {
    msg = "break outside loop at line %d";
    msg = luaO_pushfstring(ls->L, msg, gt->line);
  }
  else {
    msg = "no visible label '%s' for <goto> at line %d";
    msg = luaO_pushfstring(ls->L, msg, getstr(gt->name), gt->line);
  }
  luaK_semerror(ls, msg);
}


static void leaveblock (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  LexState *ls = fs->ls;
  int hasclose = 0;
  int stklevel = reglevel(fs, bl->nactvar);  /* level outside the block */
  removevars(fs, bl->nactvar);  /* remove block locals */
  lua_assert(bl->nactvar == fs->nactvar);  /* back to level on entry */
  if (bl->isloop)  /* has to fix pending breaks? */
    hasclose = createlabel(ls, luaS_newliteral(ls->L, "break"), 0, 0);
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
  luaK_patchtohere(fs, bl->breaklist);
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
  fs->firstlocal = ls->dyd->actvar.n;
  fs->firstlabel = ls->dyd->label.n;
  fs->bl = NULL;
  f->source = ls->source;
  luaC_objbarrier(ls->L, f, f->source);
  f->maxstacksize = 2;  /* registers 0/1 are always valid */
  enterblock(fs, bl, 0);
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
    case TK_ELSE: case TK_ELSEIF:
    case TK_END: case TK_EOS:
      return 1;
    case TK_UNTIL: return withuntil;
    default: return 0;
  }
}


static void propagate_return_type(TypeDesc*& prop, TypeDesc&& ret) {
  if (prop->getType() != VT_DUNNO) { /* had previous return path(s)? */
    if (vtIsNull(prop->getType())) {
      if (vtCanBeNullable(ret.getType())) {
        ret.setNullable();
        *prop = ret;
      }
    }
    else if (vtIsNull(ret.getType())) {
      if (vtCanBeNullable(prop->getType()))
      prop->setNullable();
    }
    else if (!prop->isCompatibleWith(ret)) {
      *prop = VT_MIXED; /* set return type to mixed */
      prop = nullptr; /* and don't update it again */
    }
  }
  else {
    *prop = ret; /* save return type */
  }
}

static bool statlist (LexState *ls, TypeDesc *prop = nullptr, bool no_ret_implies_void = false) {
  /* statlist -> { stat [';'] } */
  bool ret = false;
  while (!block_follow(ls, 1)) {
    ret = (ls->t.token == TK_RETURN);
    TypeDesc p = VT_DUNNO;
#if defined LUAI_ASSERT
    const auto levels = ls->L->nCcalls;
#endif
    statement(ls, &p);
    lua_assert(levels == ls->L->nCcalls);
    if (prop && /* do we need to propagate the return type? */
        p.getType() != VT_DUNNO) { /* is there a return path here? */
      propagate_return_type(prop, p.getType());
    }
    if (ret) break;
  }
  if (prop && /* do we need to propagate the return type? */
      !ret && /* had no return statement? */
      no_ret_implies_void) { /* does that imply a void return? */
    propagate_return_type(prop, VT_VOID); /* propagate */
  }
  return ret;
}


/*
** Continue statement. Semantically similar to "goto continue".
** Unlike break, this doesn't use labels. It tracks where to jump via BlockCnt.scopeend;
*/
static void continuestat (LexState *ls, lua_Integer backwards_surplus = 0) {
  auto line = ls->getLineNumber();
  FuncState *fs = ls->fs;
  BlockCnt *bl = fs->bl;
  int upval = 0;
  int foundloops = 0;
  luaX_next(ls); /* skip TK_CONTINUE */
  lua_Integer backwards = 1;
  if (ls->t.token == TK_INT) {
    backwards = ls->t.seminfo.i;
    if (backwards == 0) {
      throwerr(ls, "expected number of blocks to skip, found '0'", "unexpected '0'", line);
    }
    luaX_next(ls);
  }
  backwards += backwards_surplus;
  while (bl) {
    if (!bl->isloop) { /* not a loop, continue search */
      upval |= bl->upval; /* amend upvalues for closing. */
      bl = bl->previous; /* jump back current blocks to find the loop */
    }
    else { /* found a loop */
      if (--backwards == 0) { /* this is our loop */
        break;
      }
      else { /* continue search */
        upval |= bl->upval;
        bl = bl->previous;
        ++foundloops;
      }
    }
  }
  if (bl) {
    if (upval) luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0); /* close upvalues */
    luaK_concat(fs, &bl->scopeend, luaK_jump(fs));
  }
  else {
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


/* Switch logic partially inspired by Paige Marie DePol from the Lua mailing list. */
static void caselist (LexState *ls) {
  while (gett(ls) != TK_CASE
      && gett(ls) != TK_DEFAULT
      && gett(ls) != TK_END
    ) {
    if (gett(ls) == TK_PCONTINUE
        || gett(ls) == TK_CONTINUE
        ) {
      continuestat(ls, 1);
    }
    else {
      statement(ls);
    }
  }
  ls->laststat.token = TK_EOS;  /* We don't want warnings for trailing control flow statements. */
}


static void fieldsel (LexState *ls, expdesc *v) {
  /* fieldsel -> ['.' | ':'] NAME */
  FuncState *fs = ls->fs;
  expdesc key;
  luaK_exp2anyregup(fs, v);
  luaX_next(ls);  /* skip the dot or colon */
  codename(ls, &key);
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


static void recfield (LexState *ls, ConsControl *cc) {
  /* recfield -> (NAME | '['exp']') = exp */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  if (ls->t.token == TK_NAME) {
    checklimit(fs, cc->nh, MAX_INT, "items in a constructor");
    codename(ls, &key);
  }
  else  /* ls->t.token == '[' */
    yindex(ls, &key);
  cc->nh++;
  checknext(ls, '=');
  tab = *cc->t;
  luaK_indexed(fs, &tab, &key);
  expr(ls, &val);
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;  /* free registers */
}

static void prenamedfield(LexState* ls, ConsControl* cc, const char* name) {
  FuncState* fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  codestring(&key, luaX_newstring(ls, name));
  cc->nh++;
  luaX_next(ls); /* skip name token */
  checknext(ls, '=');
  tab = *cc->t;
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
    luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);  /* flush */
    cc->na += cc->tostore;
    cc->tostore = 0;  /* no more items pending */
  }
}


static void lastlistfield (FuncState *fs, ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (hasmultret(cc->v.k)) {
    luaK_setmultret(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.info, cc->na, LUA_MULTRET);
    cc->na--;  /* do not count last expression (unknown number of elements) */
  }
  else {
    if (cc->v.k != VVOID)
      luaK_exp2nextreg(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);
  }
  cc->na += cc->tostore;
}


static void listfield (LexState *ls, ConsControl *cc) {
  /* listfield -> exp */
  expr(ls, &cc->v);
  cc->tostore++;
}


static void body (LexState *ls, expdesc *e, int ismethod, int line, TypeDesc *prop = nullptr);
static void funcfield (LexState *ls, struct ConsControl *cc, bool ismethod) {
  /* funcfield -> function NAME funcargs */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc tab, key, val;
  cc->nh++;
  luaX_next(ls); /* skip TK_FUNCTION */
  codename(ls, &key);
  tab = *cc->t;
  luaK_indexed(fs, &tab, &key);
  body(ls, &val, ismethod, ls->getLineNumber());
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;  /* free registers */
}


static void field (LexState *ls, ConsControl *cc) {
  /* field -> listfield | recfield | funcfield */
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", "function");
    ss.push("stat", "static");
  }
  switch(ls->t.token) {
    case TK_NAME: {  /* may be 'listfield', 'recfield' or static 'funcfield' */
      if (strcmp(ls->t.seminfo.ts->contents, "static") != 0) {
        if (luaX_lookahead(ls) != '=')  /* expression? */
          listfield(ls, cc);
        else
          recfield(ls, cc);
      }
      else { /* static function */
        luaX_next(ls);
        check(ls, TK_FUNCTION);
        funcfield(ls, cc, false);
      }
      break;
    }
    case '[': {
      recfield(ls, cc);
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
      if (ls->t.IsReservedNonValue()) {
        prenamedfield(ls, cc, luaX_reserved2str(ls->t.token));
      } else {
        listfield(ls, cc);
      }
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
  check_match(ls, '}', '{', line);
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t->u.info, cc.na, cc.nh);
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
  luaK_settablesize(fs, pc, v->u.info, cc.na, cc.nh);
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
  luaK_settablesize(fs, pc, v->u.info, cc.na, cc.nh);
}


static TString *checkextends (LexState *ls) {
  TString *parent = nullptr;
  if (ls->shouldSuggest()) {
    SuggestionsState ss(ls);
    ss.push("stat", "extends");
    ss.push("stat", "function");
    ss.push("stat", "static");
    ss.push("stat", "end");
  }
  if (ls->t.token == TK_EXTENDS) {
    luaX_next(ls);
    parent = str_checkname(ls, true);
  }
  ls->parent_classes.emplace(parent);
  return parent;
}

static void applyextends (LexState *ls, expdesc *v, TString *parent, int line) {
  FuncState *fs = ls->fs;

  expdesc f;
  singlevar(ls, &f, luaS_newliteral(ls->L, "Pluto_operator_extends"));
  luaK_exp2nextreg(fs, &f);

  expdesc args = *v;
  luaK_exp2nextreg(fs, &args);
  singlevar(ls, &args, parent);

  lua_assert(f.k == VNONRELOC);
  int base = f.u.info;  /* base register for call */
  luaK_exp2nextreg(fs, &args);  /* close last argument */
  int nparams = fs->freereg - (base + 1);
  init_exp(&f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base + 1;
}


static void classexpr (LexState *ls, expdesc *t) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  ConsControl cc;
  luaK_code(fs, 0);  /* space for extra arg. */
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VNONRELOC, fs->freereg);  /* table will be at stack top */
  luaK_reserveregs(fs, 1);
  init_exp(&cc.v, VVOID, 0);  /* no value (yet) */
  while (ls->t.token != TK_END) {
    lua_assert(cc.v.k == VVOID || cc.tostore > 0);
    if (ls->t.token == '}') break;
    closelistfield(fs, &cc);
    field(ls, &cc);
    (testnext(ls, ',') || testnext(ls, ';'));
  }
#ifdef PLUTO_COMPATIBLE_CLASS
  check_match(ls, TK_END, TK_PCLASS, line);
#else
  check_match(ls, TK_END, TK_CLASS, line);
#endif
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t->u.info, cc.na, cc.nh);
}


static void classstat (LexState *ls) {
  auto line = ls->getLineNumber();
  luaX_next(ls); /* skip 'class' */

  expdesc v;
  singlevar(ls, &v);

  TString *parent = checkextends(ls);

  expdesc t;
  classexpr(ls, &t);
  check_readonly(ls, &v);
  luaK_storevar(ls->fs, &v, &t);
  luaK_fixline(ls->fs, line);

  lua_assert(ls->getParentClass() == parent);
  ls->parent_classes.pop();

  if (parent)
    applyextends(ls, &v, parent, line);
}


static void localclass (LexState *ls) {
  auto line = ls->getLineNumber();
  TString *name = str_checkname(ls, true);
  TString *parent = checkextends(ls);

  new_localvar(ls, name, ls->getLineNumber());

  expdesc t;
  classexpr(ls, &t);

  adjust_assign(ls, 1, 1, &t);
  adjustlocalvars(ls, 1);

  lua_assert(ls->getParentClass() == parent);
  ls->parent_classes.pop();

  if (parent) {
    expdesc v;
    singlevar(ls, &v, name);
    applyextends(ls, &v, parent, line);
  }
}

/* }====================================================================== */


static void setvararg (FuncState *fs, int nparams) {
  fs->f->is_vararg = 1;
  luaK_codeABC(fs, OP_VARARGPREP, nparams, 0, 0);
}


enum expflags {
  E_NO_COLON = 1 << 0,
  E_NO_CALL  = 1 << 1,
};

static void simpleexp (LexState *ls, expdesc *v, int flags = 0, TypeDesc *prop = nullptr);
static void simpleexp_with_unary_support (LexState *ls, expdesc *v) {
  if (testnext(ls, '-')) { /* Negative constant? */
    check(ls, TK_INT);
    init_exp(v, VKINT, 0);
    v->u.ival = (ls->t.seminfo.i * -1);
    luaX_next(ls);
  }
  else {
    testnext(ls, '+'); /* support pseudo-unary '+' */
    simpleexp(ls, v, E_NO_COLON);
  }
}


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


static void parlist (LexState *ls, std::vector<size_t>* fallbacks = nullptr, TString** varargname = nullptr) {
  /* parlist -> [ {NAME ','} (NAME | '...') ] */
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int nparams = 0;
  int isvararg = 0;
  if (ls->t.token != ')' && ls->t.token != '|') {  /* is 'parlist' not empty? */
    do {
      if (isnametkn(ls, true)) {
        auto parname = str_checkname(ls, true);
        auto parhint = gettypehint(ls);
        new_localvar(ls, parname, parhint);
        if (fallbacks) {
          if (testnext(ls, '=')) {
            fallbacks->emplace_back(luaX_getpos(ls));
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


static void compoundassign(LexState *ls, expdesc *v, BinOpr op);
static void body (LexState *ls, expdesc *e, int ismethod, int line, TypeDesc *prop) {
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
  std::vector<size_t> fallbacks{};
  TString* varargname = nullptr;
  parlist(ls, &fallbacks, &varargname);
  checknext(ls, ')');
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
      auto pc = nilable.u.info;

      expdesc fallback;
      expr(ls, &fallback);
      luaK_setoneret(fs, &fallback);
      singlevaraux(fs, vd->vd.name, &nilable, 0);
      luaK_storevar(fs, &nilable, &fallback);

      luaK_patchtohere(fs, pc);
      leavelevel(ls);
    }
    ++fallback_idx;
  }
  luaX_setpos(ls, saved_pos);
  if (varargname) {
    enterlevel(ls);
    new_localvar(ls, varargname);

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
    luaK_settablesize(fs, pc, t.u.info, cc.na, cc.nh);

    adjust_assign(ls, 1, 1, &t);
    adjustlocalvars(ls, 1);
    leavelevel(ls);
  }
  TypeDesc rethint = gettypehint(ls, true);
  TypeDesc p = VT_DUNNO;
  statlist(ls, &p, true);
  if (rethint.getType() != VT_DUNNO && /* has type hint for return type? */
      p.getType() != VT_DUNNO && /* return type is known? */
      !rethint.isCompatibleWith(p)) { /* incompatible? */
    std::string err = "function was hinted to return ";
    err.append(rethint.toString());
    err.append(" but actually returns ");
    err.append(p.toString());
    throw_warn(ls, err.c_str(), line, WT_TYPE_MISMATCH);
  }
  if (prop) { /* propagate type of function */
    *prop = VT_FUNC;
    prop->proto = new_fs.f;
    prop->retn = p.primitive;
    int vidx = new_fs.firstlocal;
    for (lu_byte i = 0; i != prop->getNumTypedParams(); ++i) {
      prop->params[i] = ls->dyd->actvar.arr[vidx].vd.hint.primitive;
      ++vidx;
    }
  }
  new_fs.f->lastlinedefined = ls->getLineNumber();
  check_match(ls, TK_END, TK_FUNCTION, line);
  codeclosure(ls, e);
  close_func(ls);
  ls->popContext(PARCTX_BODY);
}


/*
** Lambda implementation.
** Shorthands lambda expressions into `function (...) return ... end`.
** The '|' token was chosen because it's not commonly used as an unary operator in programming.
** The '->' arrow syntax looked more visually appealing than a colon. It also plays along with common lambda tokens.
*/
static void lambdabody (LexState *ls, expdesc *e, int line) {
  FuncState new_fs;
  BlockCnt bl;
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = line;
  open_func(ls, &new_fs, &bl);
  checknext(ls, '|');
  parlist(ls);
  checknext(ls, '|');
  checknext(ls, '-');
  checknext(ls, '>');
  expr(ls, e);
  luaK_ret(&new_fs, luaK_exp2anyreg(&new_fs, e), 1);
  new_fs.f->lastlinedefined = ls->getLineNumber();
  codeclosure(ls, e);
  close_func(ls);
}


static void expr_propagate(LexState *ls, expdesc *v, TypeDesc& td) {
  expr(ls, v, &td);
  exp_propagate(ls, *v, td);
}

static int explist (LexState *ls, expdesc *v, std::vector<TypeDesc>& prop) {
  /* explist -> expr { ',' expr } */
  int n = 1;  /* at least one expression */
  expr_propagate(ls, v, prop.emplace_back(VT_DUNNO));
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(ls->fs, v);
    expr_propagate(ls, v, prop.emplace_back(VT_DUNNO));
    n++;
  }
  return n;
}

static void explist_nonlinear_arg (LexState *ls, expdesc *v, size_t tidx, TypeDesc& td) {
  if (tidx == 0) {
    init_exp(v, VNIL, 0);
    td = VT_NIL;
  }
  else {
    luaX_setpos(ls, tidx);
    expr_propagate(ls, v, td);
  }
}

static void explist_nonlinear (LexState *ls, expdesc *v, const std::vector<size_t>& argtis, std::vector<TypeDesc>& prop) {
  prop.reserve(argtis.size());
  explist_nonlinear_arg(ls, v, argtis.at(0), prop.emplace_back(VT_DUNNO));
  for (size_t i = 1; i != argtis.size(); ++i) {
    luaK_exp2nextreg(ls->fs, v);
    explist_nonlinear_arg(ls, v, argtis.at(i), prop.emplace_back(VT_DUNNO));
  }
}

static int explist (LexState *ls, expdesc *v, TypeDesc *prop = nullptr) {
  /* explist -> expr { ',' expr } */
  int n = 1;  /* at least one expression */
  expr(ls, v, prop);
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(ls->fs, v);
    expr(ls, v);
    n++;
  }
  return n;
}

static void funcargs (LexState *ls, expdesc *f, int line, TypeDesc *funcdesc = nullptr) {
  ls->pushContext(PARCTX_FUNCARGS);
  FuncState *fs = ls->fs;
  expdesc args;
  std::vector<TypeDesc> argdescs;
  int base, nparams;
  switch (ls->t.token) {
    case '(': {  /* funcargs -> '(' [ explist ] ')' */
      luaX_next(ls);
      if (ls->t.token == ')')  /* arg list is empty? */
        args.k = VVOID;
      else {
        bool is_named = false;
        if (ls->t.token != TK_EOS) {
          luaX_next(ls); /* skip name */
          is_named = (ls->t.token == '=');
          luaX_prev(ls); /* back to name */
        }
        if (is_named) {
          if (!funcdesc) {
            luaX_syntaxerror(ls, "can't used named arguments here because the function was not found at parse-time");
          }
          std::vector<size_t> argtis{};
          argtis.resize(funcdesc->getNumParams());
          do {
            TString *pname = str_checkname(ls, true);
            int pi = funcdesc->findParamByName(pname);
            if (pi == -1) {
              throwerr(ls, luaO_fmt(ls->L, "function does not have a %s parameter", pname->contents), "unknown parameter");
            }
            checknext(ls, '=');
            argtis.at(pi) = luaX_getpos(ls);
            skip_over_simpleexp_within_parenlist(ls);
          } while (testnext(ls, ','));
          const auto tidx = luaX_getpos(ls);
          explist_nonlinear(ls, &args, argtis, argdescs);
          luaX_setpos(ls, tidx);
        }
        else {
          explist(ls, &args, argdescs);
        }
        if (hasmultret(args.k))
          luaK_setmultret(fs, &args);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    case '{': {  /* funcargs -> constructor */
      argdescs = { TypeDesc{ VT_TABLE } };
      constructor(ls, &args);
      break;
    }
    case TK_STRING: {  /* funcargs -> STRING */
      argdescs = { TypeDesc{ VT_STR } };
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
      const PrimitiveType& param_hint = funcdesc->params[i];
      if (param_hint.getType() == VT_DUNNO)
        continue; /* skip parameters without type hint */
      TypeDesc arg = VT_NIL;
      if (i < (int)argdescs.size()) {
        arg = argdescs.at(i);
        if (arg.getType() == VT_DUNNO)
          continue; /* skip arguments without propagated type */
      }
      if (!param_hint.isCompatibleWith(arg.primitive)) {
        std::string err = "Function's ";;
        err.append(funcdesc->proto->locvars[i].varname->contents, funcdesc->proto->locvars[i].varname->size());
        err.append(" parameter was type-hinted as ");
        err.append(param_hint.toString());
        err.append(" but provided with ");
        err.append(arg.toString());
        throw_warn(ls, err.c_str(), "argument type mismatch", line, WT_TYPE_MISMATCH);
      }
    }
    const auto expected = funcdesc->getNumParams();
    const auto received = (int)argdescs.size();
    if (!funcdesc->proto->is_vararg && expected < received) {  /* Too many arguments? */
      auto suffix = expected == 1 ? "" : "s"; // Omit plural suffixes when the noun is singular.
      throw_warn(ls,
        "too many arguments",
          luaO_fmt(ls->L, "expected %d argument%s, got %d.", expected, suffix, received), line, WT_EXCESSIVE_ARGUMENTS);
      --ls->L->top.p;
    }
  }
  lua_assert(f->k == VNONRELOC);
  base = f->u.info;  /* base register for call */
  if (hasmultret(args.k))
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
  ls->popContext(PARCTX_FUNCARGS);
}




/*
** {======================================================================
** Expression parsing
** =======================================================================
*/


/*
** Safe navigation is entirely accredited to SvenOlsen.
** http://lua-users.org/wiki/SvenOlsen
*/
static void safe_navigation(LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  luaK_exp2nextreg(fs, v);
  luaK_codeABC(fs, OP_TEST, v->u.info, NO_REG, 0 );
  {
    int old_free = fs->freereg;             
    int vreg = v->u.info;
    int j = luaK_jump(fs);
    expdesc key;
    switch(ls->t.token) {
      case '[': {
        luaX_next(ls);  /* skip the '[' */
        if (ls->t.token == '-') {
          expr(ls, &key);
          switch (key.k) {
            case VKINT: {
              key.u.ival *= -1;
              break;
            }
            case VKFLT: {
              key.u.nval *= -1;
              break;
            }
            default: {
              throwerr(ls, "unexpected symbol during navigation.", "unary '-' on non-numeral type.");
            }
          }
        }
        else expr(ls, &key);
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
      default: {
        luaX_syntaxerror(ls, "unexpected symbol");
      }
    }
    luaK_exp2nextreg(fs, v);
    fs->freereg = old_free;
    if (v->u.info != vreg) {
      luaK_codeABC(fs, OP_MOVE, vreg, v->u.info, 0);
      v->u.info = vreg;
    }
    luaK_patchtohere(fs, j);
  }
}


struct StringChain {
  LexState* ls;
  expdesc* v;

  StringChain(LexState* ls, expdesc* v) noexcept
    : ls(ls), v(v)
  {
    enterlevel(ls);
    v->k = VVOID;
  }

  ~StringChain() noexcept {
    if (v->k == VVOID) { /* ensure we produce at least an empty string */
      codestring(v, luaS_new(ls->L, ""));
    }
    leavelevel(ls);
  }

  void add(const char* str) noexcept {
    if (v->k == VVOID) { /* first chain entry? */
      codestring(v, luaS_new(ls->L, str));
    } else {
      luaK_infix(ls->fs, OPR_CONCAT, v);
      expdesc v2;
      codestring(&v2, luaS_new(ls->L, str));
      luaK_posfix(ls->fs, OPR_CONCAT, v, &v2, ls->getLineNumber());
    }
  }

  void addVar(const char* varname) noexcept {
    if (v->k == VVOID) { /* first chain entry? */
      singlevarinner(ls, luaS_new(ls->L, varname), v);
    } else {
      luaK_infix(ls->fs, OPR_CONCAT, v);
      expdesc v2;
      singlevarinner(ls, luaS_new(ls->L, varname), &v2);
      luaK_posfix(ls->fs, OPR_CONCAT, v, &v2, ls->getLineNumber());
    }
  }
};


static void fstring (LexState *ls, expdesc *v) {
  auto str = ls->t.seminfo.ts->toCpp();

  StringChain sc(ls, v);
  for (size_t i = 0;; ) {
    size_t del = str.find('{', i);
    auto chunk = str.substr(i, del - i);
    if (!chunk.empty()) {
      sc.add(chunk.c_str());
    }
    if (del == std::string::npos) {
      break;
    }
    ++del;
    size_t del2 = str.find('}', del);
    if (del2 == std::string::npos) {
      luaX_syntaxerror(ls, "Improper $-string with unterminated varname");
      break;
    }
    auto varname_len = (del2 - del);
    auto varname = str.substr(del, varname_len);
    del += varname_len + 1;
    sc.addVar(varname.c_str());
    i = del;
  }

  luaX_next(ls); /* skip string */
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
      simpleexp(ls, &argexp, E_NO_COLON);
      switch (argexp.k) {
        case VKSTR:
          lua_pushlstring(L, argexp.u.strval->contents, argexp.u.strval->size());
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
    default: {
      luaX_syntaxerror(ls, "unexpected return value in constexpr_call");
    }
  }
  lua_pop(L, 1);
}


static bool check_constexpr_call (LexState *ls, expdesc *v, const char *name, lua_CFunction f) {
  if (strcmp(ls->t.seminfo.ts->contents, name) == 0) {
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

static void const_expr (LexState *ls, expdesc *v) {
  switch (ls->t.token) {
    case TK_NAME: {
      const Pluto::Preloaded* lib = nullptr;
      for (const auto& preloaded : Pluto::all_preloaded) {
        if (strcmp(preloaded->name, ls->t.seminfo.ts->contents) == 0) {
          lib = preloaded;
          break;
        }
      }
      if (lib != nullptr) {
        luaX_next(ls); /* skip TK_NAME */
        checknext(ls, '.');
        check(ls, TK_NAME);
        lua_CFunction f = NULL;
        for (auto reg = &lib->funcs[0]; reg->name; ++reg) {
          if (strcmp(reg->name, ls->t.seminfo.ts->contents) == 0) {
            f = reg->func;
            break;
          }
        }
        if (f == NULL) {
          throwerr(ls, luaO_fmt(ls->L, "%s is not a member of %s", ls->t.seminfo.ts->contents, lib->name), "unknown function.");
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
        )
      {
        throwerr(ls, luaO_fmt(ls->L, "%s is not available in constant expression", ls->t.seminfo.ts->contents), "unrecognized name.");
      }
      return;
    }
    default: {
      const char *token = luaX_token2str(ls, ls->t.token);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void enumexp (LexState *ls, expdesc *v, TString *varname) {
  switch (ls->t.token) {
    case ':': {
      luaX_next(ls);
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("efunc", "values");
        ss.push("efunc", "names");
        ss.push("efunc", "kvmap");
        ss.push("efunc", "vkmap");
      }
      check(ls, TK_NAME);
      if (strcmp(ls->t.seminfo.ts->contents, "values") == 0) {
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
      else if (strcmp(ls->t.seminfo.ts->contents, "names") == 0) {
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
      else if (strcmp(ls->t.seminfo.ts->contents, "kvmap") == 0) {
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
      else if (strcmp(ls->t.seminfo.ts->contents, "vkmap") == 0) {
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
        throwerr(ls, luaO_fmt(ls->L, "%s is not a member of enums", ls->t.seminfo.ts->contents), "unknown member.");
      }
      return;
    }
    case '.': {
      const EnumDesc* ed = &ls->enums.at((size_t)v->u.ival);
      luaX_next(ls);
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        for (const auto& e : ed->enumerators) {
          ss.push("eprop", e.name->contents, std::to_string(e.value));
        }
      }
      check(ls, TK_NAME);
      for (const auto& e : ed->enumerators) {
        if (eqstr(e.name->contents, ls->t.seminfo.ts->contents)) {
          init_exp(v, VKINT, 0);
          v->u.ival = e.value;
          luaX_next(ls);
          return;
        }
      }
      throwerr(ls, luaO_fmt(ls->L, "%s is not a member of %s", ls->t.seminfo.ts->contents, varname->contents), "unknown member.");
      return;
    }
    default: {
      const char *token = luaX_token2str(ls, ls->t.token);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void parentexp (LexState *ls, expdesc *v) {
  if (testnext(ls, ':')) {
    if (ls->getParentClass() == nullptr)
      luaX_syntaxerror(ls, "attempt to use 'parent' outside of a class that inherits from another class");

    auto line = ls->getLineNumber();

    singlevar(ls, v, ls->getParentClass());
    luaK_exp2nextreg(ls->fs, v);

    expdesc key;
    codename(ls, &key);
    luaK_indexed(ls->fs, v, &key);
    luaK_exp2nextreg(ls->fs, v);

    expdesc first_arg;
    singlevar(ls, &first_arg, luaS_newliteral(ls->L, "self"));
    luaK_exp2nextreg(ls->fs, &first_arg);

    funcargs(ls, v, line);
  }
  else {
    singlevar(ls, v, luaS_newliteral(ls->L, "self"));
    expdesc key;
    codestring(&key, luaS_newliteral(ls->L, "__parent"));
    luaK_indexed(ls->fs, v, &key);
  }
}


static void primaryexp (LexState *ls, expdesc *v) {
  /* primaryexp -> NAME | '(' expr ')' */
  if (isnametkn(ls)) {
    TString *varname = str_checkname(ls);
    singlevar(ls, v, varname);
    if (v->k == VENUM) {
      enumexp(ls, v, varname);
    }
    return;
  }
  switch (ls->t.token) {
    case '(': {
      int line = ls->getLineNumber();
      luaX_next(ls);
      expr(ls, v);
      check_match(ls, ')', '(', line);
      luaK_dischargevars(ls->fs, v);
      return;
    }
    case '}':
    case '{': { // Unfinished table constructors.
       if (ls->t.token == '{') {
         throwerr(ls, "unfinished table constructor", "did you mean to close with '}'?");
       }
       else {
         throwerr(ls, "unfinished table constructor", "did you mean to enter with '{'?");
       }
       return;
    }
    case '|': { // Potentially mistyped lambda expression. People may confuse '->' with '=>'.
      while (testnext(ls, '|') || testnext(ls, TK_NAME) || testnext(ls, ','));
      throwerr(ls, "unexpected symbol", "impromper or stranded lambda expression.");
      return;
    }
    case '$': {
      luaX_next(ls); /* skip '$' */
      if (ls->t.token == TK_STRING) {
        fstring(ls, v);
      }
      else {
        const_expr(ls, v);
      }
      return;
    }
#ifndef PLUTO_COMPATIBLE_PARENT
    case TK_PARENT:
#endif
    case TK_PPARENT: {
      luaX_next(ls);
      parentexp(ls, v);
      return;
    }
    default: {
      const char *token = luaX_token2str(ls, ls->t.token);
      throwerr(ls, luaO_fmt(ls->L, "unexpected symbol near %s", token), "unexpected symbol.");
    }
  }
}


static void expsuffix (LexState* ls, expdesc* v, int flags, TypeDesc* prop);

static void suffixedexp (LexState *ls, expdesc *v, int flags = 0, TypeDesc *prop = nullptr) {
  /* suffixedexp ->
       primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
  primaryexp(ls, v);
  expsuffix(ls, v, flags, prop);
}

static void expsuffix (LexState *ls, expdesc *v, int flags, TypeDesc* prop) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();
  for (;;) {
    switch (ls->t.token) {
      case '?': {  /* safe navigation or ternary */
        luaX_next(ls); /* skip '?' */
        if (gett(ls) != '[' && gett(ls) != '.') {
          /* it's a ternary but we have to deal with that later */
          luaX_prev(ls); /* unskip '?' */
          return; /* back to primaryexp */
        }
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
        if (flags & E_NO_COLON) {
          return;
        }
        expdesc key;
        luaX_next(ls);
        codename(ls, &key);
        luaK_self(fs, v, &key);
        funcargs(ls, v, line);
        break;
      }
      case '(': case TK_STRING: case '{': {  /* funcargs */
        if (flags & E_NO_CALL) {
          return;
        }
        Vardesc *vd;
        TypeDesc *funcdesc = nullptr;
        if (v->k == VLOCAL) {
          vd = getlocalvardesc(ls->fs, v->u.var.vidx);
        _funcdesc_from_vd:
          if (vd->vd.prop.getType() == VT_FUNC) { /* just in case... */
            funcdesc = &vd->vd.prop;
            if (prop) { /* propagate return type */
              *prop = vd->vd.prop.retn;
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
                if (vd->vd.ridx == idx) {
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
        funcargs(ls, v, line, funcdesc);
        break;
      }
      default: return;
    }
  }
}


int cond (LexState *ls);
static void ifexpr (LexState *ls, expdesc *v) {
  /*
  ** Patch published by Ryota Hirose.
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
  luaK_exp2reg(fs, v, reg);
  luaK_patchtohere(fs, escape);
}


static void newexpr (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();

  luaX_next(ls);

  singlevar(ls, v, luaS_newliteral(ls->L, "Pluto_operator_new"));
  luaK_exp2nextreg(fs, v);

  expdesc first_arg;
  expr(ls, &first_arg, nullptr, E_NO_CALL);
  luaK_exp2nextreg(fs, &first_arg);

  funcargs(ls, v, line);
}


static void instanceof (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();

  singlevar(ls, v, luaS_newliteral(ls->L, "Pluto_operator_instanceof"));
  luaK_exp2nextreg(fs, v);

  expdesc args;
  singlevar(ls, &args);
  luaK_exp2nextreg(fs, &args);
  luaX_next(ls);
  singlevar(ls, &args);

  lua_assert(v->k == VNONRELOC);
  int base = v->u.info;  /* base register for call */
  luaK_exp2nextreg(fs, &args);  /* close last argument */
  int nparams = fs->freereg - (base + 1);
  init_exp(v, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base + 1;
}


static void simpleexp (LexState *ls, expdesc *v, int flags, TypeDesc *prop) {
  /* simpleexp -> FLT | INT | STRING | NIL | TRUE | FALSE | ... |
                  constructor | FUNCTION body | suffixedexp */
  if (ls->t.token != TK_EOS) {
    luaX_next(ls);
    const bool is_instanceof = (ls->t.token == TK_INSTANCEOF);
    luaX_prev(ls);
    if (is_instanceof) {
      instanceof(ls, v);
      return;
    }
  }
  switch (ls->t.token) {
    case TK_FLT: {
      if (prop) *prop = VT_FLT;
      init_exp(v, VKFLT, 0);
      v->u.nval = ls->t.seminfo.r;
      luaX_next(ls);
      break;
    }
    case TK_INT: {
      if (prop) *prop = VT_INT;
      init_exp(v, VKINT, 0);
      v->u.ival = ls->t.seminfo.i;
      luaX_next(ls);
      break;
    }
    case TK_STRING: {
      if (prop) *prop = VT_STR;
      codestring(v, ls->t.seminfo.ts);
      luaX_next(ls);
      break;
    }
    case TK_NIL: {
      if (prop) *prop = VT_NIL;
      init_exp(v, VNIL, 0);
      luaX_next(ls);
      break;
    }
    case TK_TRUE: {
      if (prop) *prop = VT_BOOL;
      init_exp(v, VTRUE, 0);
      luaX_next(ls);
      break;
    }
    case TK_FALSE: {
      if (prop) *prop = VT_BOOL;
      init_exp(v, VFALSE, 0);
      luaX_next(ls);
      break;
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
      if (prop) *prop = VT_TABLE;
      constructor(ls, v);
      break;
    }
    case TK_FUNCTION: {
      luaX_next(ls);
      body(ls, v, 0, ls->getLineNumber(), prop);
      return;
    }
    case '|': {
      lambdabody(ls, v, ls->getLineNumber());
      return;
    }
#ifndef PLUTO_COMPATIBLE_NEW
    case TK_NEW:
#endif
    case TK_PNEW: {
      if (prop) *prop = VT_TABLE;
      newexpr(ls, v);
      break;
    }
#ifndef PLUTO_COMPATIBLE_CLASS
    case TK_CLASS:
#endif
    case TK_PCLASS: {
      if (prop) *prop = VT_TABLE;
      luaX_next(ls); /* skip 'class' */
      classexpr(ls, v);
      return;
    }
    default: {
      suffixedexp(ls, v, flags, prop);
      return;
    }
  }
  expsuffix(ls, v, flags, prop);
}


static void inexpr (LexState *ls, expdesc *v) {
  expdesc v2;
  checknext(ls, TK_IN);
  luaK_exp2nextreg(ls->fs, v);
  lua_assert(v->k == VNONRELOC);
  int base = v->u.info;
  simpleexp(ls, &v2);
  luaK_dischargevars(ls->fs, &v2);
  luaK_exp2nextreg(ls->fs, &v2);
  luaK_codeABC(ls->fs, OP_IN, v->u.info, v2.u.info, 0);
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
    case TK_NE: return OPR_NE;
    case TK_EQ: return OPR_EQ;
    case '<': return OPR_LT;
    case TK_LE: return OPR_LE;
    case '>': return OPR_GT;
    case TK_GE: return OPR_GE;
    case TK_AND: return OPR_AND;
    case TK_OR: return OPR_OR;
    case TK_COAL: return OPR_COAL;
    case TK_POW: return OPR_POW;  /* '**' operator support */
    default: return OPR_NOBINOPR;
  }
}


static void prefixplusplus (LexState *ls, expdesc *v) {
  int line = ls->getLineNumber();
  luaX_next(ls); /* skip second '+' */
  singlevar(ls, v); /* variable name */
  FuncState *fs = ls->fs;
  expdesc e = *v, v2;
  if (v->k != VLOCAL) {  /* complex lvalue, use a temporary register. linear perf incr. with complexity of lvalue */
    luaK_reserveregs(fs, fs->freereg-fs->nactvar);
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
   {2, 2}, {1, 1}, {1, 1}    /* and, or, ?? */
};

#define UNARY_PRIORITY	12  /* priority for unary operators */


/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where 'binop' is any binary operator with a priority higher than 'limit'
*/
static BinOpr subexpr (LexState *ls, expdesc *v, int limit, TypeDesc *prop = nullptr, int flags = 0) {
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
      prefixplusplus(ls, v);
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
      inexpr(ls, v);
      if (prop) *prop = VT_BOOL;
    }
  }
  /* expand while operators have priorities higher than 'limit' */
  op = getbinopr(ls->t.token);
  while (op != OPR_NOBINOPR && priority[op].left > limit) {
    expdesc v2;
    BinOpr nextop;
    int line = ls->getLineNumber();
    luaX_next(ls);  /* skip operator */
    luaK_infix(ls->fs, op, v);
    /* read sub-expression with higher priority */
    nextop = subexpr(ls, &v2, priority[op].right, nullptr, flags);
    luaK_posfix(ls->fs, op, v, &v2, line);
    op = nextop;
  }
  leavelevel(ls);
  return op;  /* return first untreated operator */
}


static void expr (LexState *ls, expdesc *v, TypeDesc *prop, int flags) {
  luaX_checkspecial(ls);
  subexpr(ls, v, 0, prop, flags);
  if (testnext(ls, '?')) { /* ternary expression? */
    int escape = NO_JUMP;
    v->normaliseFalse();
    luaK_goiftrue(ls->fs, v);
    int condition = v->f;
    expr(ls, v, nullptr, true);
    auto fs = ls->fs;
    auto reg = luaK_exp2anyreg(fs, v);
    luaK_concat(fs, &escape, luaK_jump(fs));
    luaK_patchtohere(fs, condition);
    checknext(ls, ':');
    expr(ls, v);
    luaK_exp2reg(fs, v, reg);
    luaK_patchtohere(fs, escape);
    if (prop) *prop = VT_MIXED; /* reset propagated type */
  }
}

/* }==================================================================== */



/*
** {======================================================================
** Rules for Statements
** =======================================================================
*/


static void block (LexState *ls) {
  /* block -> statlist */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, 0);
  statlist(ls);
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
  gets the supported binary compound operation (if any)
  gives OPR_NOBINOPR if the operation does not have compound support.
  returns a status (0 false, 1 true) and takes a pointer to set.
  this allows for seamless conditional implementation, avoiding a getcompoundop call for every Lua assignment.
*/
static int getcompoundop (lua_Integer i, BinOpr *op) {
  switch (i) {
    case TK_CCAT: {
      *op = OPR_CONCAT;
      return 1;       /* concatenation */
    }
    case TK_CADD: {
      *op = OPR_ADD;  /* addition */
      return 1;
    }
    case TK_CSUB: {
      *op = OPR_SUB;  /* subtraction */
      return 1;
    }
    case TK_CMUL: {
      *op = OPR_MUL;  /* multiplication */
      return 1;
    }
    case TK_CMOD: {
      *op = OPR_MOD;  /* modulo */
      return 1;
    }
    case TK_CDIV: {
      *op = OPR_DIV;  /* float division */
      return 1;
    }
    case TK_CPOW: {
      *op = OPR_POW;  /* power */
      return 1;
    }
    case TK_CIDIV: {
      *op = OPR_IDIV;  /* integer division */
      return 1;
    }
    case TK_CBOR: {
      *op = OPR_BOR;  /* bitwise OR */
      return 1;
    }
    case TK_CBAND: {
      *op = OPR_BAND;  /* bitwise AND */
      return 1;
    }
    case TK_CBXOR: {
      *op = OPR_BXOR;  /* bitwise XOR */
      return 1;
    }
    case TK_CSHL: {
      *op = OPR_SHL;  /* shift left */
      return 1; 
    }
    case TK_CSHR: {
      *op = OPR_SHR;  /* shift right */
      return 1;
    }
    case TK_COAL: {
      *op = OPR_COAL;
      return 1;
    }
    default: {
      *op = OPR_NOBINOPR;
      return 0;
    }
  }
}

/* 
  compound assignment function
  determines the binary operation to perform depending on lexer state tokens (ls->lasttoken)
  resets the lexer state token
  reserves N registers (where N = local variables on stack)
  preforms binary operation and assignment
*/ 
static void compoundassign(LexState *ls, expdesc *v, BinOpr op) {
  luaX_next(ls);
  int line = ls->getLineNumber();
  FuncState *fs = ls->fs;
  expdesc e = *v, v2;
  if (v->k != VLOCAL) {  /* complex lvalue, use a temporary register. linear perf incr. with complexity of lvalue */
    luaK_reserveregs(fs, fs->freereg-fs->nactvar);
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
    BinOpr op;  /* binary operation from lexer state */
    if (getcompoundop(ls->t.seminfo.i, &op) != 0) {  /* is there a saved binop? */
      check_condition(ls, nvars == 1, "unsupported tuple assignment");
      compoundassign(ls, &lh->v, op);  /* perform binop & assignment */
      return;  /* avoid default */
    }
    else if (testnext(ls, '=')) { /* no requested binop, continue */
      TypeDesc prop = VT_DUNNO;
      ParserContext ctx = ((nvars == 1) ? PARCTX_CREATE_VAR : PARCTX_CREATE_VARS);
      ls->pushContext(ctx);
      int nexps = explist(ls, &e, &prop);
      ls->popContext(ctx);
      if (nexps != nvars)
        adjust_assign(ls, nvars, nexps, &e);
      else {
        luaK_setoneret(ls->fs, &e);  /* close last expression */
        if (lh->v.k == VLOCAL) { /* assigning to a local variable? */
          exp_propagate(ls, e, prop);
          process_assign(ls, getlocalvardesc(ls->fs, lh->v.u.var.vidx), prop, line);
        }
        luaK_storevar(ls->fs, &lh->v, &e);
        return;  /* avoid default */
      }
    }
  }
  init_exp(&e, VNONRELOC, ls->fs->freereg-1);  /* default assignment */
  luaK_storevar(ls->fs, &lh->v, &e);
}

int cond (LexState *ls) {
  /* cond -> exp */
  expdesc v;
  expr(ls, &v);  /* read condition */
  v.normaliseFalse();
  luaK_goiftrue(ls->fs, &v);
  return v.f;
}


static void lgoto(LexState *ls, TString *name) {
  FuncState *fs = ls->fs;
  int line = ls->getLineNumber();
  Labeldesc *lb = findlabel(ls, name);
  if (lb == NULL)  /* no label? */
    /* forward jump; will be resolved when the label is declared */
    newgotoentry(ls, name, line, luaK_jump(fs));
  else {  /* found a label */
    /* backward jump; will be resolved here */
    int lblevel = reglevel(fs, lb->nactvar);  /* label level */
    if (luaY_nvarstack(fs) > lblevel)  /* leaving the scope of a variable? */
      luaK_codeABC(fs, OP_CLOSE, lblevel, 0, 0);
    /* create jump and link it to the label */
    luaK_patchlist(fs, luaK_jump(fs), lb->pc);
  }
}

static void gotostat (LexState *ls) {
  lgoto(ls, str_checkname(ls));
}


/*
** Break statement. Very similiar to `continue` usage, but it jumps slightly more forward.
**
** Implementation Detail:
**   Unlike normal Lua, it has been reverted from a label implementation back into a mix between a label & patchlist implementation.
**   This allows reusage of the existing "continue" implementation, which has been time-tested extensively by now.
*/
static void breakstat (LexState *ls) {
  auto line = ls->getLineNumber();
  FuncState *fs = ls->fs;
  BlockCnt *bl = fs->bl;
  int upval = 0;
  luaX_next(ls); /* skip TK_BREAK */
  lua_Integer backwards = 1;
  if (ls->t.token == TK_INT) {
    backwards = ls->t.seminfo.i;
    if (backwards == 0) {
      throwerr(ls, "expected number of blocks to skip, found '0'", "unexpected '0'", line);
    }
    luaX_next(ls);
  }
  while (bl) {
    if (!bl->isloop) { /* not a loop, continue search */
      upval |= bl->upval; /* amend upvalues for closing. */
      bl = bl->previous; /* jump back current blocks to find the loop */
    }
    else { /* found a loop */
      if (--backwards == 0) { /* this is our loop */
        break;
      }
      else { /* continue search */
        upval |= bl->upval;
        bl = bl->previous;
      }
    };
  }
  if (bl) {
    if (upval) luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0); /* close upvalues */
    luaK_concat(fs, &bl->breaklist, luaK_jump(fs));
  }
  else {
    throwerr(ls, "break can't skip that many blocks", "try a smaller number", line);
  }
}


// Test the next token to see if it's either 'token1' or 'token2'.
inline bool testnext2 (LexState *ls, int token1, int token2) {
  return testnext(ls, token1) || testnext(ls, token2);
}


static void casecond(LexState* ls, int case_line, expdesc& lcase) {
  expr(ls, &lcase, nullptr, E_NO_COLON);
  checknext(ls, ':');
}


struct SwitchCase {
  size_t tidx;
  int pc;
};

static void switchstat (LexState *ls, int line) {
  int switchToken = gett(ls);
  luaX_next(ls); // Skip switch statement.
  testnext(ls, '(');

  FuncState* fs = ls->fs;
  BlockCnt sbl;
  enterblock(fs, &sbl, 1);

  expdesc crtl, save, first;
  expr(ls, &crtl);
  luaK_exp2nextreg(ls->fs, &crtl);
  init_exp(&save, VLOCAL, crtl.u.info);
  testnext(ls, ')');
  checknext(ls, TK_DO);
  new_localvarliteral(ls, "(switch control value)"); // Save control value into a local.
  adjustlocalvars(ls, 1);

  TString* const begin_switch = luaS_newliteral(ls->L, "pluto_begin_switch");
  TString* const end_switch = luaS_newliteral(ls->L, "pluto_end_switch");
  TString* default_case = nullptr;
  int default_pc;

  if (gett(ls) == TK_CASE) {
    int case_line = ls->getLineNumber();

    luaX_next(ls); /* Skip 'case' */

    first = save;

    luaK_infix(fs, OPR_NE, &first);
    expdesc lcase;
    casecond(ls, case_line, lcase);
    luaK_posfix(fs, OPR_NE, &first, &lcase, ls->getLineNumber());

    caselist(ls);
  }
  else {
    first.k = VVOID;
    newgotoentry(ls, begin_switch, ls->getLineNumber(), luaK_jump(fs)); // goto begin_switch
  }

  std::vector<SwitchCase> cases{};

  while (gett(ls) != TK_END) {
    auto case_line = ls->getLineNumber();
    if (gett(ls) == TK_DEFAULT) {
      luaX_next(ls); /* Skip 'default' */
      checknext(ls, ':');
      if (default_case != nullptr)
        throwerr(ls, "switch statement already has a default case", "second default case", case_line);
      default_case = luaS_newliteral(ls->L, "pluto_default_case");
      default_pc = luaK_getlabel(fs);
      createlabel(ls, default_case, ls->getLineNumber(), block_follow(ls, 0));
      caselist(ls);
    }
    else {
      checknext(ls, TK_CASE);
      cases.emplace_back(SwitchCase{ luaX_getpos(ls), luaK_getlabel(fs) });
      skip_until(ls, ':'); /* skip over casecond */
      checknext(ls, ':');
      caselist(ls);
    }
  }

  /* handle possible fallthrough, don't loop infinitely */
  newgotoentry(ls, end_switch, ls->getLineNumber(), luaK_jump(fs)); // goto end_switch

  if (first.k != VVOID) {
    luaK_patchtohere(fs, first.u.info);
  }
  else {
    createlabel(ls, begin_switch, ls->getLineNumber(), block_follow(ls, 0)); // ::begin_switch::
  }

  /* prune cases that lead to default case */
  if (default_case) {
    for (auto i = cases.begin(); i != cases.end(); ) {
      if (i->pc == default_pc) {
        i = cases.erase(i);
      }
      else {
        ++i;
      }
    }
  }

  expdesc test;
  for (auto& c : cases) {
    test = save;
    luaK_infix(fs, OPR_EQ, &test);
    auto pos = luaX_getpos(ls);
    luaX_setpos(ls, c.tidx);
    expdesc cc;
    casecond(ls, ls->getLineNumber(), cc);
    luaX_setpos(ls, pos);
    luaK_posfix(fs, OPR_EQ, &test, &cc, ls->getLineNumber());
    luaK_patchlist(fs, test.u.info, c.pc);
  }

  if (default_case != nullptr)
    lgoto(ls, default_case);

  createlabel(ls, end_switch, ls->getLineNumber(), block_follow(ls, 0)); // ::end_switch::

  check_match(ls, TK_END, switchToken, line);
  leaveblock(fs);
}


static void enumstat (LexState *ls) {
  /* enumstat -> ENUM [[CLASS] NAME] BEGIN NAME ['=' INT] { ',' NAME ['=' INT] } END */

  luaX_next(ls); /* skip 'enum' */

  EnumDesc *ed = nullptr;
  bool is_enum_class = false;
  if (gett(ls) != TK_BEGIN) { /* enum has name (and possibly modifier)? */
    if (gett(ls) == TK_CLASS) {
      is_enum_class = true;
      luaX_next(ls);
    }
    auto vidx = new_localvar(ls, str_checkname(ls, true), ls->getLineNumber());
    auto var = getlocalvardesc(ls->fs, vidx);
    var->vd.kind = RDKENUM;
    setivalue(&var->k, ls->enums.size());
    ed = &ls->enums.emplace_back(EnumDesc{});
    ls->fs->nactvar++;
  }

  const auto line_begin = ls->getLineNumber();
  checknext(ls, TK_BEGIN); /* ensure we have 'begin' */

  lua_Integer i = 1;
  while (gett(ls) != TK_END) {
    TString *name = str_checkname(ls, true);
    int vidx;
    if (!is_enum_class) {
      vidx = new_localvar(ls, name, ls->getLineNumber());
    }
    if (testnext(ls, '=')) {
      expdesc v;
      simpleexp_with_unary_support(ls, &v);
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
      i++;
      ls->fs->nactvar++;
    }
    if (gett(ls) != ',') break;
    luaX_next(ls);
  }

  check_match(ls, TK_END, TK_BEGIN, line_begin);
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
  createlabel(ls, name, line, block_follow(ls, 0));
}


static void whilestat (LexState *ls, int line) {
  /* whilestat -> WHILE cond DO block END */
  FuncState *fs = ls->fs;
  int whileinit;
  int condexit;
  BlockCnt bl;
  luaX_next(ls);  /* skip WHILE */
  whileinit = luaK_getlabel(fs);
  condexit = cond(ls);
  enterblock(fs, &bl, 1);
  checknext(ls, TK_DO);
  block(ls);
  luaK_jumpto(fs, whileinit);
  luaK_patchlist(fs, bl.scopeend, whileinit);
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
  enterblock(fs, &bl1, 1);  /* loop block */
  enterblock(fs, &bl2, 0);  /* scope block */
  luaX_next(ls);  /* skip REPEAT */
  statlist(ls);
  luaK_patchtohere(fs, bl1.scopeend);
  if (testnext(ls, TK_UNTIL)) {
    condexit = cond(ls);  /* read condition (inside scope block) */
  }
  else {
    error_expected(ls, TK_UNTIL);
  }
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
static void forbody (LexState *ls, int base, int line, int nvars, int isgen) {
  /* forbody -> DO block */
  static const OpCode forprep[2] = {OP_FORPREP, OP_TFORPREP};
  static const OpCode forloop[2] = {OP_FORLOOP, OP_TFORLOOP};
  BlockCnt bl;
  FuncState *fs = ls->fs;
  int prep, endfor;
  checknext(ls, TK_DO);
  prep = luaK_codeABx(fs, forprep[isgen], base, 0);
  enterblock(fs, &bl, 0);  /* scope for declared variables */
  adjustlocalvars(ls, nvars);
  luaK_reserveregs(fs, nvars);
  block(ls);
  leaveblock(fs);  /* end of scope for declared variables */
  fixforjump(fs, prep, luaK_getlabel(fs), 0);
  luaK_patchtohere(fs, bl.previous->scopeend);
  if (isgen) {  /* generic for? */
    luaK_codeABC(fs, OP_TFORCALL, base, 0, nvars);
    luaK_fixline(fs, line);
  }
  endfor = luaK_codeABx(fs, forloop[isgen], base, 0);
  fixforjump(fs, endfor, prep + 1, 1);
  luaK_fixline(fs, line);
}


static void fornum (LexState *ls, TString *varname, int line) {
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
  forbody(ls, base, line, 1, 0);
}


static void forlist (LexState *ls, TString *indexname) {
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
  forbody(ls, base, line, nvars - 4, 1);
}


static void forvlist (LexState *ls, TString *valname) {
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
  new_localvar(ls, valname);
  nvars++;

  line = ls->getLineNumber();
  adjust_assign(ls, 4, explist(ls, &e), &e);
  adjustlocalvars(ls, 4);  /* control variables */
  marktobeclosed(fs);  /* last control var. must be closed */
  luaK_checkstack(fs, 3);  /* extra space to call generator */

  checknext(ls, TK_AS);
  luaX_next(ls); /* skip valname */

  forbody(ls, base, line, nvars - 4, 1);
}


static void forstat (LexState *ls, int line) {
  /* forstat -> FOR (fornum | forlist) END */
  FuncState *fs = ls->fs;
  TString *varname = nullptr;
  BlockCnt bl;
  enterblock(fs, &bl, 1);  /* scope for loop and control variables */
  luaX_next(ls);  /* skip 'for' */

  /* determine if this is a for-as loop */
  auto sp = luaX_getpos(ls);
  for (; ls->t.token != TK_IN && ls->t.token != TK_DO && ls->t.token != TK_EOS; luaX_next(ls)) {
    if (ls->t.token == TK_AS) {
      luaX_next(ls);
      varname = str_checkname(ls);
      break;
    }
  }
  luaX_setpos(ls, sp);

  if (varname == nullptr) {
    varname = str_checkname(ls);  /* first variable name */
    switch (ls->t.token) {
      case '=': {
        fornum(ls, varname, line);
        break;
      }
      case ',': case TK_IN: {
        forlist(ls, varname);
        break;
      }
      default: {
        luaX_syntaxerror(ls, "'=' or 'in' expected");
      }
    }
  }
  else {
    forvlist(ls, varname);
  }
  check_match(ls, TK_END, TK_FOR, line);
  leaveblock(fs);  /* loop scope ('break' jumps to this point) */
}


static void test_then_block (LexState *ls, int *escapelist, TypeDesc *prop) {
  /* test_then_block -> [IF | ELSEIF] cond THEN block */
  BlockCnt bl;
  FuncState *fs = ls->fs;
  expdesc v;
  int jf;  /* instruction to skip 'then' code (if condition is false) */
  luaX_next(ls);  /* skip IF or ELSEIF */
  expr(ls, &v);  /* read condition */
  if (v.k == VNIL || v.k == VFALSE)
    throw_warn(ls, "unreachable code", "this condition will never be truthy.", WT_UNREACHABLE_CODE);
  checknext(ls, TK_THEN);
  if (ls->t.token == TK_BREAK && luaX_lookahead(ls) != TK_INT) {  /* 'if x then break' and not 'if x then break int' ? */
    ls->laststat.token = TK_BREAK;
    int line = ls->getLineNumber();
    luaK_goiffalse(ls->fs, &v);  /* will jump if condition is true */
    luaX_next(ls);  /* skip 'break' */
    enterblock(fs, &bl, 0);  /* must enter block before 'goto' */
    newgotoentry(ls, luaS_newliteral(ls->L, "break"), line, v.t);
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
    enterblock(fs, &bl, 0);
    jf = v.f;
  }
  statlist(ls, prop);  /* 'then' part */
  leaveblock(fs);
  if (ls->t.token == TK_ELSE ||
      ls->t.token == TK_ELSEIF)  /* followed by 'else'/'elseif'? */
    luaK_concat(fs, escapelist, luaK_jump(fs));  /* must jump over it */
  luaK_patchtohere(fs, jf);
}


static void ifstat (LexState *ls, int line, TypeDesc *prop = nullptr) {
  /* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
  FuncState *fs = ls->fs;
  int escapelist = NO_JUMP;  /* exit list for finished parts */
  test_then_block(ls, &escapelist, prop);  /* IF cond THEN block */
  while (ls->t.token == TK_ELSEIF)
    test_then_block(ls, &escapelist, prop);  /* ELSEIF cond THEN block */
  if (testnext(ls, TK_ELSE))
    block(ls);  /* 'else' part */
  check_match(ls, TK_END, TK_IF, line);
  luaK_patchtohere(fs, escapelist);  /* patch escape list to 'if' end */
}


static void localfunc (LexState *ls) {
  expdesc b;
  FuncState *fs = ls->fs;
  int fvar = fs->nactvar;  /* function's variable index */
  new_localvar(ls, str_checkname(ls, true));  /* new local variable */
  adjustlocalvars(ls, 1);  /* enter its scope */
  TypeDesc funcdesc;
  body(ls, &b, 0, ls->getLineNumber(), &funcdesc);  /* function created in next register */
  getlocalvardesc(fs, fvar)->vd.prop = std::move(funcdesc);
  /* debug information will only see the variable after this point! */
  localdebuginfo(fs, fvar)->startpc = fs->pc;
}


static int getlocalattribute (LexState *ls) {
  /* ATTRIB -> ['<' Name '>'] */
  if (testnext(ls, '<')) {
    const char *attr = getstr(str_checkname(ls));
    checknext(ls, '>');
    if (strcmp(attr, "const") == 0)
      return RDKCONST;  /* read-only variable */
    else if (strcmp(attr, "constexpr") == 0)
      return RDKCONSTEXP;  /* read-only variable */
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


static void destructuring (LexState *ls) {
  auto line = ls->getLineNumber();
  std::vector<TString*> props{};
  luaX_next(ls); /* skip '{' */
  do {
    props.emplace_back(str_checkname(ls));
  } while (testnext(ls, ','));
  check_match(ls, '}', '{', line);
  checknext(ls, '=');

  /* begin scope of the locals we want to create */
  for (const auto& prop : props) {
    new_localvar(ls, prop, line);
  }
  expdesc e;
  e.k = VVOID;
  adjust_assign(ls, (int)props.size(), 0, &e);
  adjustlocalvars(ls, (int)props.size());

  /* get table */
  expdesc t;
  expr(ls, &t);

  /* special case for destructuring a single field only, can be done in-place */
  if (t.k == VNONRELOC && props.size() == 1) {
    expdesc k, l;
    codestring(&k, props.at(0));
    luaK_indexed(ls->fs, &t, &k);
    singlevar(ls, &l, props.at(0));
    luaK_storevar(ls->fs, &l, &t);
    return;
  }

  /* ensure table has a place to stay */
  TString* temporary = nullptr;
  if (t.k != VLOCAL) {
    temporary = luaS_newliteral(ls->L, "(temporary)");
    new_localvar(ls, temporary, line);
    adjust_assign(ls, 1, 1, &t);
    adjustlocalvars(ls, 1);
  }

  /* assign locals */
  for (const auto& prop : props) {
    expdesc e, k, l;
    if (temporary)
      singlevar(ls, &e, temporary);
    else
      e = t;
    codestring(&k, prop);
    luaK_indexed(ls->fs, &e, &k);
    singlevar(ls, &l, prop);
    luaK_storevar(ls->fs, &l, &e);
  }

  /* release table */
  if (temporary) {
    removevars(ls->fs, ls->fs->nactvar - 1);
  }
}


static void localstat (LexState *ls) {
  if (ls->t.token == '{') {
    destructuring(ls);
    return;
  }
  /* stat -> LOCAL NAME ATTRIB { ',' NAME ATTRIB } ['=' explist] */
  FuncState *fs = ls->fs;
  int toclose = -1;  /* index of to-be-closed variable (if any) */
  Vardesc *var;  /* last variable */
  int vidx, kind;  /* index and kind of last variable */
  TypeDesc hint = VT_DUNNO;
  int nvars = 0;
  int nexps;
  expdesc e;
  auto line = ls->getLineNumber(); /* in case we need to emit a warning */
  size_t starting_tidx = ls->tidx; /* code snippets on tuple assignments can have inaccurate line readings because the parser skips lines until it can close the statement */
  bool is_constexpr = false;
  do {
    if (is_constexpr)
      luaK_semerror(ls, "<constexpr> must only be used on the last variable in local list");
    vidx = new_localvar(ls, str_checkname(ls, true), line);
    hint = gettypehint(ls);
    kind = getlocalattribute(ls);
    var = getlocalvardesc(fs, vidx);
    var->vd.kind = kind;
    var->vd.hint = hint;
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
    nvars++;
  } while (testnext(ls, ','));
  std::vector<TypeDesc> tds;
  if (testnext(ls, '=')) {
    ParserContext ctx = ((nvars == 1) ? PARCTX_CREATE_VAR : PARCTX_CREATE_VARS);
    ls->pushContext(ctx);
    nexps = explist(ls, &e, tds);
    ls->popContext(ctx);
  }
  else {
    e.k = VVOID;
    nexps = 0;
    process_assign(ls, var, VT_NIL, line);
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
      for (TypeDesc& td : tds) {
        exp_propagate(ls, e, td);
        process_assign(ls, getlocalvardesc(fs, vidx), td, line);
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


static void funcstat (LexState *ls, int line) {
  /* funcstat -> FUNCTION funcname body */
  int ismethod;
  expdesc v, b;
  luaX_next(ls);  /* skip FUNCTION */
  ismethod = funcname(ls, &v);
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
  if (ls->t.token == '=' || ls->t.token == ',') { /* stat -> assignment ? */
    v.prev = NULL;
    restassign(ls, &v, 1);
  }
  else {  /* stat -> func */
    Instruction *inst;
    check_condition(ls, v.v.k == VCALL, "syntax error");
    inst = &getinstruction(fs, &v.v);
    SETARG_C(*inst, 1);  /* call statement uses no results */
  }
}


static void retstat (LexState *ls, TypeDesc *prop) {
  /* stat -> RETURN [explist] [';'] */
  FuncState *fs = ls->fs;
  expdesc e;
  int nret;  /* number of values being returned */
  int first = luaY_nvarstack(fs);  /* first slot to be returned */
  if (block_follow(ls, 1) || ls->t.token == ';'
    || ls->t.token == TK_CASE || ls->t.token == TK_DEFAULT
  ) {
    nret = 0;  /* return no values */
    if (prop) *prop = VT_VOID;
  }
  else {
    nret = explist(ls, &e, prop);  /* optional return values */
    if (hasmultret(e.k)) {
      luaK_setmultret(fs, &e);
      if (e.k == VCALL && nret == 1 && !fs->bl->insidetbc) {  /* tail call? */
        SET_OPCODE(getinstruction(fs,&e), OP_TAILCALL);
        lua_assert(GETARG_A(getinstruction(fs,&e)) == luaY_nvarstack(fs));
      }
      nret = LUA_MULTRET;  /* return all values */
    }
    else {
      if (nret == 1)  /* only one single value? */
        first = luaK_exp2anyreg(fs, &e);  /* can use original slot */
      else {  /* values must go to the top of the stack */
        luaK_exp2nextreg(fs, &e);
        lua_assert(nret == fs->freereg - first);
      }
    }
  }
  luaK_ret(fs, first, nret);
  testnext(ls, ';');  /* skip optional semicolon */
}


static void statement (LexState *ls, TypeDesc *prop) {
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
    ss.push("stat", "enum");
    if (ls->fs->bl->previous)
      ss.push("stat", "end");
    ss.pushLocals();
    return;
  }
  int line = ls->getLineNumber();
  if ((ls->laststat.IsEscapingToken()
    || (ls->laststat.Is(TK_GOTO) && !ls->findWithinLine(line, luaX_lookbehind(ls).seminfo.ts->toCpp()))) /* Don't warn if this statement is the goto's label. */
    && ls->t.token != ';') /* Don't warn if this is only a semicolon */
  {
    throw_warn(ls,
      "unreachable code",
        luaO_fmt(ls->L, "this code comes after an escaping %s statement.", luaX_token2str(ls, ls->laststat.token)), WT_UNREACHABLE_CODE);
    ls->L->top.p--;
  }
  if (ls->t.token != ';')
    ls->laststat.token = ls->t.token;
  enterlevel(ls);
  switch (ls->t.token) {
    case ';': {  /* stat -> ';' (empty statement) */
      luaX_next(ls);  /* skip ';' */
      break;
    }
    case TK_IF: {  /* stat -> ifstat */
      ifstat(ls, line, prop);
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
      forstat(ls, line);
      break;
    }
    case TK_REPEAT: {  /* stat -> repeatstat */
      repeatstat(ls);
      break;
    }
    case TK_FUNCTION: {  /* stat -> funcstat */
      funcstat(ls, line);
      break;
    }
#ifndef PLUTO_COMPATIBLE_CLASS
    case TK_CLASS:
#endif
    case TK_PCLASS: {
      classstat(ls);
      break;
    }
    case TK_LOCAL: {  /* stat -> localstat */
      luaX_next(ls);  /* skip LOCAL */
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("stat", "function");
        ss.push("stat", "class");
        ss.push("stat", "pluto_class");
        return;
      }
      if (testnext(ls, TK_FUNCTION))  /* local function? */
        localfunc(ls);
#ifdef PLUTO_COMPATIBLE_CLASS
      else if (testnext(ls, TK_PCLASS))
#else
      else if (testnext(ls, TK_CLASS) || testnext(ls, TK_PCLASS))
#endif
        localclass(ls);
      else
        localstat(ls);
      break;
    }
#ifndef PLUTO_COMPATIBLE_EXPORT
    case TK_EXPORT:
#endif
    case TK_PEXPORT: {
      if (ls->fs->bl->previous)
        luaX_syntaxerror(ls, "Attempt to use 'export' outside of global scope");
      luaX_next(ls);  /* skip EXPORT */
      if (ls->shouldSuggest()) {
        SuggestionsState ss(ls);
        ss.push("stat", "function");
        ss.push("stat", "class");
        ss.push("stat", "pluto_class");
        break;
      }
      if (testnext(ls, TK_FUNCTION)) {
        ls->export_symbols.emplace_back(str_checkname(ls, true));
        luaX_prev(ls);
        localfunc(ls);
      }
#ifdef PLUTO_COMPATIBLE_CLASS
      else if (testnext(ls, TK_PCLASS)) {
#else
      else if (testnext(ls, TK_CLASS) || testnext(ls, TK_PCLASS)) {
#endif
        ls->export_symbols.emplace_back(str_checkname(ls, true));
        luaX_prev(ls);
        localclass(ls);
      }
      else {
        ls->export_symbols.emplace_back(str_checkname(ls, true));
        luaX_prev(ls);
        localstat(ls);
      }
      break;
    }
    case TK_DBCOLON: {  /* stat -> label */
      luaX_next(ls);  /* skip double colon */
      labelstat(ls, str_checkname(ls), line);
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
#ifndef PLUTO_COMPATIBLE_CONTINUE
    case TK_CONTINUE:
#endif
    case TK_PCONTINUE: {
      continuestat(ls);
      break;
    }
    case TK_GOTO: {  /* stat -> 'goto' NAME */
      luaX_next(ls);  /* skip 'goto' */
      gotostat(ls);
      break;
    }
#ifndef PLUTO_COMPATIBLE_SWITCH
    case TK_SWITCH:
#endif
    case TK_PSWITCH: {
      switchstat(ls, line);
      break;
    }
#ifndef PLUTO_COMPATIBLE_ENUM
    case TK_ENUM:
#endif
    case TK_PENUM: {
      enumstat(ls);
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
  bool uses_new = false;
  bool uses_extends = false;
  bool uses_instanceof = false;

  /* discover what operators are used */
  for (const auto& t : ls->tokens) {
    switch (t.token) {
#ifndef PLUTO_COMPATIBLE_NEW
      case TK_NEW:
#endif
      case TK_PNEW:
        uses_new = true;
        break;
      case TK_EXTENDS:
        uses_extends = true;
        break;
      case TK_INSTANCEOF:
        uses_instanceof = true;
        break;
    }
  }

  /* inject implementers */
  if (uses_new || uses_extends || uses_instanceof) {
    /* capture state */
    std::vector<Token> tokens = std::move(ls->tokens);

    ls->tokens = {}; /* avoid use of moved warning */

    if (uses_new) {
      // local function Pluto_operator_new(mt, ...)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "Pluto_operator_new")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_DOTS));
      ls->tokens.emplace_back(Token(')'));

      //   if type(mt) ~= "table" then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "type")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token(')'));
      ls->tokens.emplace_back(Token(TK_NE));
      ls->tokens.emplace_back(Token(TK_STRING, luaS_newliteral(ls->L, "table")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     error "'new' used on non-table value"
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "error")));
      ls->tokens.emplace_back(Token(TK_STRING, luaS_newliteral(ls->L, "'new' used on non-table value")));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   local t = {}
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token('{'));
      ls->tokens.emplace_back(Token('}'));

      //   setmetatable(t, mt)
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "setmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token(')'));

      //   if not mt.__index or mt.__parent then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NOT));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__index")));
      ls->tokens.emplace_back(Token(TK_OR));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__parent")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     mt.__index = mt
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__index")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   if t.__construct then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__construct")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //     t:__construct(...)
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(':'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__construct")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_DOTS));
      ls->tokens.emplace_back(Token(')'));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   return t
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }
    if (uses_extends) {
      // local function Pluto_operator_extends(c, p)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "Pluto_operator_extends")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "c")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "p")));
      ls->tokens.emplace_back(Token(')'));

      // setmetatable(c, { __index = p })
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "setmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "c")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token('{'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__index")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "p")));
      ls->tokens.emplace_back(Token('}'));
      ls->tokens.emplace_back(Token(')'));

      //   c.__parent = p
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "c")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__parent")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "p")));

      // end
      ls->tokens.emplace_back(Token(TK_END));
    }
    if (uses_instanceof) {
      // local function Pluto_operator_instanceof(t, mt)
      ls->tokens.emplace_back(Token(TK_LOCAL));
      ls->tokens.emplace_back(Token(TK_FUNCTION));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "Pluto_operator_instanceof")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(','));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token(')'));

      //   t = getmetatable(t)
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "getmetatable")));
      ls->tokens.emplace_back(Token('('));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(')'));

      //   while t do
      ls->tokens.emplace_back(Token(TK_WHILE));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(TK_DO));

      //     if t == mt then
      ls->tokens.emplace_back(Token(TK_IF));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token(TK_EQ));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "mt")));
      ls->tokens.emplace_back(Token(TK_THEN));

      //       return true
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_TRUE));

      //     end
      ls->tokens.emplace_back(Token(TK_END));

      //     t = t.__parent
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token('='));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "t")));
      ls->tokens.emplace_back(Token('.'));
      ls->tokens.emplace_back(Token(TK_NAME, luaS_newliteral(ls->L, "__parent")));

      //   end
      ls->tokens.emplace_back(Token(TK_END));

      //   return false
      ls->tokens.emplace_back(Token(TK_RETURN));
      ls->tokens.emplace_back(Token(TK_FALSE));

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
  const bool ret = statlist(ls);  /* parse main body */
  check(ls, TK_EOS);
  if (!ls->export_symbols.empty()) {
    if (ret) {
      luaX_prev(ls);
      luaX_syntaxerror(ls, "'export' used but main body already returns something");
    }
    enterlevel(ls);
    size_t i = 0;
    expdesc t;
    newtable(ls, &t, [ls, &i](expdesc *k, expdesc *v) {
      if (i == ls->export_symbols.size())
        return false;
      codestring(k, ls->export_symbols.at(i));
      singlevar(ls, v, ls->export_symbols.at(i));
      ++i;
      return true;
    });
    luaK_ret(ls->fs, luaK_exp2anyreg(fs, &t), 1);
    leavelevel(ls);
  }
  close_func(ls);
}


LClosure *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff,
                       Dyndata *dyd, const char *name, int firstchar) {
  LexState lexstate;
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
  mainfunc(&lexstate, &funcstate);
  lua_assert(!funcstate.prev && funcstate.nups == 1 && !lexstate.fs);
  /* all scopes should be correctly finished */
  lua_assert(dyd->actvar.n == 0 && dyd->gt.n == 0 && dyd->label.n == 0);
  L->top.p--;  /* remove scanner's table */
  return cl;  /* closure is on the stack, too */
}
