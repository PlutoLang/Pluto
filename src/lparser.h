#pragma once
/*
** $Id: lparser.h $
** Lua Parser
** See Copyright Notice in lua.h
*/

#include "llimits.h"
#include "lobject.h"
#include "lzio.h"
#include "llex.h"


/*
** Shorthand for converting a token into a non-quoted string.
*/
#define t2s(lex, token) (luaX_token2str_noq(lex, token))


/*
** Expression and variable descriptor.
** Code generation for variables and expressions can be delayed to allow
** optimizations; An 'expdesc' structure describes a potentially-delayed
** variable/expression. It has a description of its "main" value plus a
** list of conditional jumps that can also produce its value (generated
** by short-circuit operators 'and'/'or').
*/

/* kinds of variables/expressions */
typedef enum {
  VVOID,  /* when 'expdesc' describes the last expression of a list,
             this kind means an empty list (so, no expression) */
  VNIL,  /* constant nil */
  VTRUE,  /* constant true */
  VFALSE,  /* constant false */
  VK,  /* constant in 'k'; info = index of constant in 'k' */
  VKFLT,  /* floating constant; nval = numerical float value */
  VKINT,  /* integer constant; ival = numerical integer value */
  VKSTR,  /* string constant; strval = TString address;
             (string is fixed by the lexer) */
  VNONRELOC,  /* expression has its value in a fixed register;
                 info = result register */
  VLOCAL,  /* local variable; var.ridx = register index;
              var.vidx = relative index in 'actvar.arr'  */
  VUPVAL,  /* upvalue variable; info = index of upvalue in 'upvalues' */
  VCONST,  /* compile-time <const> variable;
              info = absolute index in 'actvar.arr'  */
  VINDEXED,  /* indexed variable;
                ind.t = table register;
                ind.idx = key's R index */
  VINDEXUP,  /* indexed upvalue;
                ind.t = table upvalue;
                ind.idx = key's K index */
  VINDEXI, /* indexed variable with constant integer;
                ind.t = table register;
                ind.idx = key's value */
  VINDEXSTR, /* indexed variable with literal string;
                ind.t = table register;
                ind.idx = key's K index */
  VJMP,  /* expression is a test/comparison;
            info = pc of corresponding jump instruction */
  VRELOC,  /* expression can put result in any register;
              info = instruction pc */
  VCALL,  /* expression is a function call; info = instruction pc */
  VVARARG  /* vararg expression; info = instruction pc */
} expkind;

[[nodiscard]] constexpr bool vkisconst(lu_byte k) noexcept {
  return k >= VNIL && k <= VKSTR;
}

#define vkisvar(k)	(VLOCAL <= (k) && (k) <= VINDEXSTR)
#define vkisindexed(k)	(VINDEXED <= (k) && (k) <= VINDEXSTR)


typedef struct expdesc {
  expkind k;
  union {
    lua_Integer ival;    /* for VKINT */
    lua_Number nval;  /* for VKFLT */
    TString *strval;  /* for VKSTR */
    int info;  /* for generic use */
    struct {  /* for indexed variables */
      short idx;  /* index (R or "long" K) */
      lu_byte t;  /* table (register or upvalue) */
    } ind;
    struct {  /* for local variables */
      lu_byte ridx;  /* register holding the variable */
      unsigned short vidx;  /* compiler index (in 'actvar.arr')  */
    } var;
  } u;
  int t;  /* patch list of 'exit when true' */
  int f;  /* patch list of 'exit when false' */
} expdesc;


/* kinds of variables */
#define VDKREG		0   /* regular */
#define RDKCONST	1   /* constant */
#define RDKTOCLOSE	2   /* to-be-closed */
#define RDKCTC		3   /* compile-time constant */

/* types of values, for type hinting and propagation */
enum ValType : lu_byte {
  VT_DUNNO = 0,
  VT_MIXED,
  VT_NIL,
  VT_NUMBER,
  VT_BOOL,
  VT_STR,
  VT_TABLE,
  VT_FUNC,

  NUL_VAL_TYPES
};

[[nodiscard]] inline const char* vt_toString(ValType vt) noexcept {
   switch (vt)
   {
   case VT_DUNNO: return "dunno";
   case VT_MIXED: return "mixed";
   case VT_NIL: return "nil";
   case VT_NUMBER: return "number";
   case VT_BOOL: return "boolean";
   case VT_STR: return "string";
   case VT_TABLE: return "table";
   case VT_FUNC: return "function";
   case NUL_VAL_TYPES: break;
   }
   return "ERROR";
}

union Vardesc;

class TypeDesc
{
private:
  /*
  ** 4 bits for type. if function, another 4 bits for return type.
  ** type consists of 3 bits for ValType, and 1 bit for nullable.
  */
  lu_byte data;
  static_assert((NUL_VAL_TYPES - 1) <= 0b111);

  /* parameter info for functions */
  int numparams;
  int firstlocal;

public:
  TypeDesc(ValType vt, bool nullable = false)
    : data(vt | (nullable << 3))
  {
  }

  void setFunction(int numparams, int firstlocal, ValType ret_typ, bool ret_nullable) noexcept {
    this->data = VT_FUNC;
    this->data |= (ret_typ << 4);
    this->data |= (ret_nullable << 7);
    this->numparams = numparams;
    this->firstlocal = firstlocal;
  }

  [[nodiscard]] ValType getType() const noexcept {
    return (ValType)(data & 0b111);
  }

  [[nodiscard]] bool isNullable() const noexcept {
    return (data >> 3) & 1;
  }

  void setNullable() noexcept {
    data |= (1 << 3);
  }

  [[nodiscard]] ValType getReturnType() const noexcept {
    return (ValType)((data >> 4) & 0b111);
  }

  [[nodiscard]] bool isReturnNullable() const noexcept {
    return (data >> 7) & 1;
  }

  [[nodiscard]] int getNumParams() const noexcept {
      return numparams;
  }

  void fromReturn(TypeDesc& retdesc) noexcept {
    data = (retdesc.data >> 4);
  }

  [[nodiscard]] Vardesc& getParam(LexState* ls, int i) const noexcept;

  [[nodiscard]] bool isCompatibleWith(const TypeDesc& b) const noexcept {
    const auto b_t = b.getType();
    return (getType() == b_t)
        ? (isNullable() || !b.isNullable()) /* if same type, b can't be nullable if a isn't nullable */
        : (b_t == VT_NIL && isNullable()) /* if different type, b might still be compatible if a is nullable and b is nil */
        ;
  }

  [[nodiscard]] std::string toString() const {
    auto vt = getType();
    std::string str{};
    if (isNullable()) {
      str.push_back('?');
    }
    str.append(vt_toString(vt));
    if (vt == VT_FUNC) {
      auto rt = getReturnType();
      if (rt != VT_DUNNO) {
        str.push_back('(');
        if (isReturnNullable()) {
          str.push_back('?');
        }
        str.append(vt_toString(rt));
        str.push_back(')');
      }
    }
    return str;
  }
};

/* description of an active local variable */
typedef union Vardesc {
  struct {
    TValuefields;  /* constant value (if it is a compile-time constant) */
    lu_byte kind;
    TypeDesc hint;
    TypeDesc prop; /* type propagation */
    lu_byte ridx;  /* register holding the variable */
    short pidx;  /* index of the variable in the Proto's 'locvars' array */
    TString *name;  /* variable name */
    int linenumber;
  } vd;
  TValue k;  /* constant value (if any) */
} Vardesc;



/* description of pending goto statements and label statements */
typedef struct Labeldesc {
  TString *name;  /* label identifier */
  int pc;  /* position in code */
  int line;  /* line where it appeared */
  lu_byte nactvar;  /* number of active variables in that position */
  lu_byte close;  /* goto that escapes upvalues */
} Labeldesc;


/* list of labels or gotos */
typedef struct Labellist {
  Labeldesc *arr;  /* array */
  int n;  /* number of entries in use */
  int size;  /* array size */
} Labellist;


/* dynamic structures used by the parser */
typedef struct Dyndata {
  struct {  /* list of all active local variables */
    Vardesc *arr;
    int n;
    int size;
  } actvar;
  Labellist gt;  /* list of pending gotos */
  Labellist label;   /* list of active labels */
} Dyndata;


/* control of blocks */
struct BlockCnt;  /* defined in lparser.c */


/* state needed to generate code for a given function */
typedef struct FuncState {
  Proto *f;  /* current function header */
  struct FuncState *prev;  /* enclosing function */
  struct LexState *ls;  /* lexical state */
  struct BlockCnt *bl;  /* chain of current blocks */
  int pc;  /* next position to code (equivalent to 'ncode') */
  int lasttarget;   /* 'label' of last 'jump label' */
  int previousline;  /* last line that was saved in 'lineinfo' */
  int nk;  /* number of elements in 'k' */
  int np;  /* number of elements in 'p' */
  int nabslineinfo;  /* number of elements in 'abslineinfo' */
  int firstlocal;  /* index of first local var (in Dyndata array) */
  int firstlabel;  /* index of first label (in 'dyd->label->arr') */
  short ndebugvars;  /* number of elements in 'f->locvars' */
  lu_byte nactvar;  /* number of active local variables */
  lu_byte nups;  /* number of upvalues */
  lu_byte freereg;  /* first free register */
  lu_byte iwthabs;  /* instructions issued since last absolute line info */
  lu_byte needclose;  /* function needs to close upvalues when returning */
} FuncState;


LUAI_FUNC int luaY_nvarstack (FuncState *fs);
LUAI_FUNC LClosure *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff,
                                 Dyndata *dyd, const char *name, int firstchar);
