#pragma once
/*
** $Id: ldebug.h $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in lua.h
*/

#include "lstate.h"


#define pcRel(pc, p)	(cast_int((pc) - (p)->code) - 1)


/* Active Lua function (given call info) */
#define ci_func(ci)		(clLvalue(s2v((ci)->func.p)))


#define resethookcount(L)	(L->hookcount = L->basehookcount)

/*
** mark for entries in 'lineinfo' array that has absolute information in
** 'abslineinfo' array
*/
#define ABSLINEINFO	(-0x80)


/*
** MAXimum number of successive Instructions WiTHout ABSolute line
** information. (A power of two allows fast divisions.)
*/
#if !defined(MAXIWTHABS)
#define MAXIWTHABS	128
#endif


LUAI_FUNC int luaG_getfuncline (const Proto *f, int pc);
LUAI_FUNC const char *luaG_findlocal (lua_State *L, CallInfo *ci, int n,
                                                    StkId *pos);
[[noreturn]] LUAI_FUNC void luaG_typeerror (lua_State *L, const TValue *o,
                                                const char *opname);
[[noreturn]] LUAI_FUNC void luaG_callerror (lua_State *L, const TValue *o);
[[noreturn]] LUAI_FUNC void luaG_forerror (lua_State *L, const TValue *o,
                                               const char *what);
[[noreturn]] LUAI_FUNC void luaG_concaterror (lua_State *L, const TValue *p1,
                                                  const TValue *p2);
[[noreturn]] LUAI_FUNC void luaG_opinterror (lua_State *L, const TValue *p1,
                                                 const TValue *p2,
                                                 const char *msg);
[[noreturn]] LUAI_FUNC void luaG_tointerror (lua_State *L, const TValue *p1,
                                                 const TValue *p2);
[[noreturn]] LUAI_FUNC void luaG_ordererror (lua_State *L, const TValue *p1,
                                                 const TValue *p2);
[[noreturn]] LUAI_FUNC void luaG_runerror (lua_State *L, const char *fmt, ...);
LUAI_FUNC const char *luaG_addinfo (lua_State *L, const char *msg,
                                                  TString *src, int line);
[[noreturn]] LUAI_FUNC void luaG_errormsg (lua_State *L);
LUAI_FUNC int luaG_traceexec (lua_State *L, const Instruction *pc);
LUAI_FUNC int luaG_tracecall (lua_State *L);
