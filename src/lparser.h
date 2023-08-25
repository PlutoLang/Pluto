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
  VVARARG,  /* vararg expression; info = instruction pc */
  VENUM
} expkind;

[[nodiscard]] constexpr bool vkisconst(lu_byte k) noexcept {
  return k == VNIL || k == VTRUE || k == VFALSE || k == VKFLT || k == VKINT || k == VKSTR || k == VCONST;
}

#define vkisvar(k)	(VLOCAL <= (k) && (k) <= VINDEXSTR)
#define vkisindexed(k)	(VINDEXED <= (k) && (k) <= VINDEXSTR)

/* types of values, for type hinting and propagation */
enum ValType : lu_byte {
  VT_DUNNO = 0,
  VT_VOID,
  VT_NIL,
  VT_NUMBER,
  VT_INT,
  VT_FLT,
  VT_BOOL,
  VT_STR,
  VT_TABLE,
  VT_FUNC,

  NUL_VAL_TYPES
};

[[nodiscard]] inline const char* vtToString(ValType vt) {
  switch (vt) {
    case VT_DUNNO: return "dunno";
    case VT_VOID: return "void";
    case VT_NIL: return "nil";
    case VT_NUMBER: return "number";
    case VT_INT: return "int";
    case VT_FLT: return "float";
    case VT_BOOL: return "boolean";
    case VT_STR: return "string";
    case VT_TABLE: return "table";
    case VT_FUNC: return "function";
    default:;
  }
  return "ERROR";
}

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
  ValType code_primitive;

  void normaliseFalse() {
    if (k == VNIL) k = VFALSE;
  }
} expdesc;

/* kinds of variables */
#define VDKREG		0   /* regular */
#define RDKCONST	1   /* constant */
#define RDKTOCLOSE	2   /* to-be-closed */
#define RDKCTC		3   /* compile-time constant */
#define RDKCONSTEXP	4   /* [Pluto] enforced compile-time constant */
#define RDKENUM		5   /* [Pluto] named enum */

struct TypeHint;

struct TypeDesc {
  ValType type;

  /* function info */
  Proto* proto;
  TypeHint* retn;
  static constexpr int MAX_TYPED_PARAMS = 10;
  TypeHint* params[MAX_TYPED_PARAMS];

  TypeDesc() = default;

  TypeDesc(ValType type)
    : type(type)
  {
  }

  [[nodiscard]] ValType getType() const noexcept {
    return type;
  }

  [[nodiscard]] lu_byte getNumParams() noexcept {
    return proto->numparams;
  }

  [[nodiscard]] int findParamByName(TString* name) noexcept {
    for (lu_byte i = 0; i != getNumParams(); ++i) {
      if (name == proto->locvars[i].varname) {
        return i;
      }
    }
    return -1;
  }

  [[nodiscard]] lu_byte getNumTypedParams() noexcept {
    auto p = getNumParams();
    if (p >= MAX_TYPED_PARAMS)
      p = MAX_TYPED_PARAMS;
    return p;
  }

  [[nodiscard]] std::string toString() const;
};

struct TypeHint {
  TypeDesc descs[3];

  TypeHint() {
    clear();
  }

  TypeHint(ValType vt) {
    clear();
    emplaceTypeDesc(vt);
  }

  void operator=(ValType) = delete;

  void clear() noexcept {
    for (auto& desc : descs) {
      desc.type = VT_DUNNO;
    }
  }

  [[nodiscard]] bool empty() const noexcept {
    return descs[0].type == VT_DUNNO;
  }

  bool emplaceTypeDesc(TypeDesc td) {
    for (auto& desc : descs) {
      if (desc.type == VT_DUNNO) {
        desc = std::move(td);
        return true;
      }
    }
    return false;
  }

  void merge(const TypeHint& b) {
    for (auto& desc : b.descs) {
      emplaceTypeDesc(desc);
    }
  }

  [[nodiscard]] bool contains(ValType vt) const noexcept {
    for (const auto& desc : descs) {
      if (desc.type == vt) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool isCompatibleWith(const TypeDesc& td) const noexcept {
    return contains(td.type)
        || ((td.type == VT_INT || td.type == VT_FLT) && contains(VT_NUMBER));
  }

  [[nodiscard]] bool isCompatibleWith(const TypeHint& b) const noexcept {
    if (b.empty()) {
      return isNullable();
    }
    for (const auto& desc : b.descs) {
      if (!isCompatibleWith(desc)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] ValType toPrimitive() const noexcept {
    if (descs[1].type == VT_DUNNO) {
      return descs[0].type;
    }
    return VT_DUNNO;
  }

  [[nodiscard]] bool isNullable() const noexcept {
    return contains(VT_NIL);
  }

  void fixTypes() {
    if (descs[1].type != VT_DUNNO) { /* contains more than 1 type? */
      /* convert 'void' to 'nil' */
      for (auto& desc : descs) {
        if (desc.type == VT_VOID) {
          desc.type = VT_NIL;
          break;
        }
      }
    }
  }

  [[nodiscard]] std::string toString() const {
    if (empty()) {
      return "nil";
    }
    std::string str{};
    if (isNullable())
      str.push_back('?');
    for (const auto& desc : descs) {
      if (desc.type != VT_DUNNO && desc.type != VT_NIL) {
        str.append(desc.toString());
        str.push_back('|');
      }
    }
    if (!str.empty() && str.back() == '|')
      str.pop_back();
    return str;
  }
};

/* description of an active local variable */
typedef union Vardesc {
  struct {
    TValuefields;  /* constant value (if it is a compile-time constant) */
    lu_byte kind;
    TypeHint* hint;
    TypeHint* prop; /* type propagation */
    lu_byte ridx;  /* register holding the variable */
    short pidx;  /* index of the variable in the Proto's 'locvars' array */
    TString *name;  /* variable name */
    int line;
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
  int nactvar;  /* number of active local variables */
  lu_byte nups;  /* number of upvalues */
  lu_byte freereg;  /* first free register */
  lu_byte iwthabs;  /* instructions issued since last absolute line info */
  lu_byte needclose;  /* function needs to close upvalues when returning */
} FuncState;


LUAI_FUNC int luaY_nvarstack (FuncState *fs);
LUAI_FUNC LClosure *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff,
                                 Dyndata *dyd, const char *name, int firstchar);


/*
** Return the "variable description" (Vardesc) of a given variable.
** (Unless noted otherwise, all variables are referred to by their
** compiler indices.)
*/
inline Vardesc* getlocalvardesc(FuncState* fs, int vidx) {
  return &fs->ls->dyd->actvar.arr[fs->firstlocal + vidx];
}
