/*
** $Id: ljumptab.h $
** Jump Table for the Lua interpreter
** See Copyright Notice in lua.h
*/


#undef vmdispatch
#undef vmcase
#undef vmbreak

#define vmdispatch(x)     switch(x) { case OP_MOVE: goto L_OP_MOVE; case OP_LOADI: goto L_OP_LOADI; case OP_LOADF: goto L_OP_LOADF; case OP_LOADK: goto L_OP_LOADK; case OP_LOADKX: goto L_OP_LOADKX; case OP_LOADFALSE: goto L_OP_LOADFALSE; case OP_LFALSESKIP: goto L_OP_LFALSESKIP; case OP_LOADTRUE: goto L_OP_LOADTRUE; case OP_LOADNIL: goto L_OP_LOADNIL; case OP_GETUPVAL: goto L_OP_GETUPVAL; case OP_SETUPVAL: goto L_OP_SETUPVAL; case OP_GETTABUP: goto L_OP_GETTABUP; case OP_GETTABLE: goto L_OP_GETTABLE; case OP_GETI: goto L_OP_GETI; case OP_GETFIELD: goto L_OP_GETFIELD; case OP_SETTABUP: goto L_OP_SETTABUP; case OP_SETTABLE: goto L_OP_SETTABLE; case OP_SETI: goto L_OP_SETI; case OP_SETFIELD: goto L_OP_SETFIELD; case OP_NEWTABLE: goto L_OP_NEWTABLE; case OP_SELF: goto L_OP_SELF; case OP_ADDI: goto L_OP_ADDI; case OP_ADDK: goto L_OP_ADDK; case OP_SUBK: goto L_OP_SUBK; case OP_MULK: goto L_OP_MULK; case OP_MODK: goto L_OP_MODK; case OP_POWK: goto L_OP_POWK; case OP_DIVK: goto L_OP_DIVK; case OP_IDIVK: goto L_OP_IDIVK; case OP_BANDK: goto L_OP_BANDK; case OP_BORK: goto L_OP_BORK; case OP_BXORK: goto L_OP_BXORK; case OP_SHRI: goto L_OP_SHRI; case OP_SHLI: goto L_OP_SHLI; case OP_ADD: goto L_OP_ADD; case OP_SUB: goto L_OP_SUB; case OP_MUL: goto L_OP_MUL; case OP_MOD: goto L_OP_MOD; case OP_POW: goto L_OP_POW; case OP_DIV: goto L_OP_DIV; case OP_IDIV: goto L_OP_IDIV; case OP_BAND: goto L_OP_BAND; case OP_BOR: goto L_OP_BOR; case OP_BXOR: goto L_OP_BXOR; case OP_SHL: goto L_OP_SHL; case OP_SHR: goto L_OP_SHR; case OP_MMBIN: goto L_OP_MMBIN; case OP_MMBINI: goto L_OP_MMBINI; case OP_MMBINK: goto L_OP_MMBINK; case OP_UNM: goto L_OP_UNM; case OP_BNOT: goto L_OP_BNOT; case OP_NOT: goto L_OP_NOT; case OP_LEN: goto L_OP_LEN; case OP_CONCAT: goto L_OP_CONCAT; case OP_CLOSE: goto L_OP_CLOSE; case OP_TBC: goto L_OP_TBC; case OP_JMP: goto L_OP_JMP; case OP_EQ: goto L_OP_EQ; case OP_LT: goto L_OP_LT; case OP_LE: goto L_OP_LE; case OP_EQK: goto L_OP_EQK; case OP_EQI: goto L_OP_EQI; case OP_LTI: goto L_OP_LTI; case OP_LEI: goto L_OP_LEI; case OP_GTI: goto L_OP_GTI; case OP_GEI: goto L_OP_GEI; case OP_TEST: goto L_OP_TEST; case OP_TESTSET: goto L_OP_TESTSET; case OP_CALL: goto L_OP_CALL; case OP_TAILCALL: goto L_OP_TAILCALL; case OP_RETURN: goto L_OP_RETURN; case OP_RETURN0: goto L_OP_RETURN0; case OP_RETURN1: goto L_OP_RETURN1; case OP_FORLOOP: goto L_OP_FORLOOP; case OP_FORPREP: goto L_OP_FORPREP; case OP_TFORPREP: goto L_OP_TFORPREP; case OP_TFORCALL: goto L_OP_TFORCALL; case OP_TFORLOOP: goto L_OP_TFORLOOP; case OP_SETLIST: goto L_OP_SETLIST; case OP_CLOSURE: goto L_OP_CLOSURE; case OP_VARARG: goto L_OP_VARARG; case OP_VARARGPREP: goto L_OP_VARARGPREP; case OP_EXTRAARG: goto L_OP_EXTRAARG; case OP_IN: goto L_OP_IN; case NUM_OPCODES: goto L_NUM_OPCODES; }

#define vmcase(l)     L_##l:

#define vmbreak		vmfetch(); vmdispatch(GET_OPCODE(i));
