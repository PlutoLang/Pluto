/*
** $Id: ltablib.c $
** Library for Table Manipulation
** See Copyright Notice in lua.h
*/

#define ltablib_c
#define LUA_LIB

#include "lprefix.h"


#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lstate.h"
#include "ltable.h"


/*
** Operations that an object must define to mimic a table
** (some functions only need some of them)
*/
#define TAB_R	1			/* read */
#define TAB_W	2			/* write */
#define TAB_L	4			/* length */
#define TAB_RW	(TAB_R | TAB_W)		/* read/write */


#define aux_getn(L,n,w)	(checktab(L, n, (w) | TAB_L), luaL_len(L, n))


static int checkfield (lua_State *L, const char *key, int n) {
  lua_pushstring(L, key);
  return (lua_rawget(L, -n) != LUA_TNIL);
}


/*
** Check that 'arg' either is a table or can behave like one (that is,
** has a metatable with the required metamethods)
*/
static void checktab (lua_State *L, int arg, int what) {
  if (lua_type(L, arg) != LUA_TTABLE) {  /* is it not a table? */
    int n = 1;  /* number of elements to pop */
    if (lua_getmetatable(L, arg) &&  /* must have metatable */
        (!(what & TAB_R) || checkfield(L, "__index", ++n)) &&
        (!(what & TAB_W) || checkfield(L, "__newindex", ++n)) &&
        (!(what & TAB_L) || checkfield(L, "__len", ++n))) {
      lua_pop(L, n);  /* pop metatable and tested metamethods */
    }
    else
      luaL_checktype(L, arg, LUA_TTABLE);  /* force an error */
  }
}


static int tinsert (lua_State *L) {
  lua_Integer pos;  /* where to insert new element */
  lua_Integer e = aux_getn(L, 1, TAB_RW);
  e = luaL_intop(+, e, 1);  /* first empty element */
  switch (lua_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      lua_Integer i;
      pos = luaL_checkinteger(L, 2);  /* 2nd argument is the position */
      /* check whether 'pos' is in [1, e] */
      luaL_argcheck(L, (lua_Unsigned)pos - 1u < (lua_Unsigned)e, 2,
                       "position out of bounds");
      for (i = e; i > pos; i--) {  /* move up elements */
        lua_geti(L, 1, i - 1);
        lua_seti(L, 1, i);  /* t[i] = t[i - 1] */
      }
      break;
    }
    default: {
      luaL_error(L, "wrong number of arguments to 'insert'");
    }
  }
  lua_seti(L, 1, pos);  /* t[pos] = v */
  return 0;
}

static int foreach(lua_State* L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  const bool callwithkey = lua_istrue(L, 3);
  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1)) {
    lua_pushvalue(L, 2);  /* function */
    if (callwithkey) {
      lua_pushvalue(L, -3);  /* key */
      lua_pushvalue(L, -3);  /* value */
      lua_call(L, 2, 1);
    }
    else {
      lua_pushvalue(L, -2);  /* value */
      lua_call(L, 1, 1);
    }
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 2);  /* remove value and result */
  }
  return 0;
}

static int tremove (lua_State *L) {
  lua_Integer size = aux_getn(L, 1, TAB_RW);
  lua_Integer pos = luaL_optinteger(L, 2, size);
  if (pos != size)  /* validate 'pos' if given */
    /* check whether 'pos' is in [1, size + 1] */
    luaL_argcheck(L, (lua_Unsigned)pos - 1u <= (lua_Unsigned)size, 2,
                     "position out of bounds");
#ifndef PLUTO_DISABLE_LENGTH_CACHE
  lua_setcachelen(L, size - 1, 1);
#endif
  lua_geti(L, 1, pos);  /* result = t[pos] */
  for ( ; pos < size; pos++) {
    lua_geti(L, 1, pos + 1);
    lua_seti(L, 1, pos);  /* t[pos] = t[pos + 1] */
  }
  lua_pushnil(L);
  lua_seti(L, 1, pos);  /* remove entry t[pos] */
  return 1;
}


/*
** Copy elements (1[f], ..., 1[e]) into (tt[t], tt[t+1], ...). Whenever
** possible, copy in increasing order, which is better for rehashing.
** "possible" means destination after original range, or smaller
** than origin, or copying to another table.
*/
static int tmove (lua_State *L) {
  lua_Integer f = luaL_checkinteger(L, 2);
  lua_Integer e = luaL_checkinteger(L, 3);
  lua_Integer t = luaL_checkinteger(L, 4);
  int tt = !lua_isnoneornil(L, 5) ? 5 : 1;  /* destination table */
  checktab(L, 1, TAB_R);
  checktab(L, tt, TAB_W);
  if (e >= f) {  /* otherwise, nothing to move */
    lua_Integer n, i;
    luaL_argcheck(L, f > 0 || e < LUA_MAXINTEGER + f, 3,
                  "too many elements to move");
    n = e - f + 1;  /* number of elements to move */
    luaL_argcheck(L, t <= LUA_MAXINTEGER - n + 1, 4,
                  "destination wrap around");
    if (t > e || t <= f || (tt != 1 && !lua_compare(L, 1, tt, LUA_OPEQ))) {
      for (i = 0; i < n; i++) {
        lua_geti(L, 1, f + i);
        lua_seti(L, tt, t + i);
      }
    }
    else {
      for (i = n - 1; i >= 0; i--) {
        lua_geti(L, 1, f + i);
        lua_seti(L, tt, t + i);
      }
    }
  }
  lua_pushvalue(L, tt);  /* return destination table */
  return 1;
}


static void addfield (lua_State *L, luaL_Buffer *b, lua_Integer i) {
  lua_geti(L, 1, i);
  if (l_unlikely(!lua_isstring(L, -1)))
    luaL_error(L, "invalid value (%s) at index %I in table for 'concat'",
                  luaL_typename(L, -1), (LUAI_UACINT)i);
  luaL_addvalue(b);
}


static int tconcat (lua_State *L) {
  luaL_Buffer b;
  lua_Integer last = aux_getn(L, 1, TAB_R);
  size_t lsep;
  const char *sep = luaL_optlstring(L, 2, "", &lsep);
  lua_Integer i = luaL_optinteger(L, 3, 1);
  last = luaL_optinteger(L, 4, last);
  luaL_buffinit(L, &b);
  for (; i < last; i++) {
    addfield(L, &b, i);
    luaL_addlstring(&b, sep, lsep);
  }
  if (i == last)  /* add last value (if interval was not empty) */
    addfield(L, &b, i);
  luaL_pushresult(&b);
  return 1;
}


/*
** {======================================================
** Pack/unpack
** =======================================================
*/

static int tpack (lua_State *L) {
  int i;
  int n = lua_gettop(L);  /* number of elements to pack */
  lua_createtable(L, n, 1);  /* create result table */
  lua_insert(L, 1);  /* put it at index 1 */
  for (i = n; i >= 1; i--)  /* assign elements */
    lua_seti(L, 1, i);
  lua_pushinteger(L, n);
  lua_setfield(L, 1, "n");  /* t.n = number of elements */
  return 1;  /* return table */
}


static int tunpack (lua_State *L) {
  lua_Unsigned n;
  lua_Integer i = luaL_optinteger(L, 2, 1);
  lua_Integer e = luaL_opt(L, luaL_checkinteger, 3, luaL_len(L, 1));
  if (i > e) return 0;  /* empty range */
  n = (lua_Unsigned)e - i;  /* number of elements minus 1 (avoid overflows) */
  if (l_unlikely(n >= (unsigned int)INT_MAX  ||
                 !lua_checkstack(L, (int)(++n))))
    luaL_error(L, "too many results to unpack");
  for (; i < e; i++) {  /* push arg[i..e - 1] (to avoid overflows) */
    lua_geti(L, 1, i);
  }
  lua_geti(L, 1, e);  /* push last element */
  return (int)n;
}

/* }====================================================== */



/*
** {======================================================
** Quicksort
** (based on 'Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
** =======================================================
*/


/* type for array indices */
typedef unsigned int IdxT;


/*
** Produce a "random" 'unsigned int' to randomize pivot choice. This
** macro is used only when 'sort' detects a big imbalance in the result
** of a partition. (If you don't want/need this "randomness", ~0 is a
** good choice.)
*/
#if !defined(l_randomizePivot)		/* { */

#include <time.h>

/* size of 'e' measured in number of 'unsigned int's */
#define sof(e)		(sizeof(e) / sizeof(unsigned int))

/*
** Use 'time' and 'clock' as sources of "randomness". Because we don't
** know the types 'clock_t' and 'time_t', we cannot cast them to
** anything without risking overflows. A safe way to use their values
** is to copy them to an array of a known type and use the array values.
*/
static unsigned int l_randomizePivot (void) {
  clock_t c = clock();
  time_t t = time(NULL);
  unsigned int buff[sof(c) + sof(t)];
  unsigned int i, rnd = 0;
  memcpy(buff, &c, sof(c) * sizeof(unsigned int));
  memcpy(buff + sof(c), &t, sof(t) * sizeof(unsigned int));
  for (i = 0; i < sof(buff); i++)
    rnd += buff[i];
  return rnd;
}

#endif					/* } */


/* arrays larger than 'RANLIMIT' may use randomized pivots */
#define RANLIMIT	100u


static void set2 (lua_State *L, IdxT i, IdxT j) {
  lua_seti(L, 1, i);
  lua_seti(L, 1, j);
}


/*
** Return true iff value at stack index 'a' is less than the value at
** index 'b' (according to the order of the sort).
*/
static int sort_comp (lua_State *L, int a, int b) {
  if (lua_isnil(L, 2))  /* no function? */
    return lua_compare(L, a, b, LUA_OPLT);  /* a < b */
  else {  /* function */
    int res;
    lua_pushvalue(L, 2);    /* push function */
    lua_pushvalue(L, a-1);  /* -1 to compensate function */
    lua_pushvalue(L, b-2);  /* -2 to compensate function and 'a' */
    lua_call(L, 2, 1);      /* call function */
    res = lua_toboolean(L, -1);  /* get result */
    lua_pop(L, 1);          /* pop result */
    return res;
  }
}


/*
** Does the partition: Pivot P is at the top of the stack.
** precondition: a[lo] <= P == a[up-1] <= a[up],
** so it only needs to do the partition from lo + 1 to up - 2.
** Pos-condition: a[lo .. i - 1] <= a[i] == P <= a[i + 1 .. up]
** returns 'i'.
*/
static IdxT partition (lua_State *L, IdxT lo, IdxT up) {
  IdxT i = lo;  /* will be incremented before first use */
  IdxT j = up - 1;  /* will be decremented before first use */
  /* loop invariant: a[lo .. i] <= P <= a[j .. up] */
  for (;;) {
    /* next loop: repeat ++i while a[i] < P */
    while ((void)lua_geti(L, 1, ++i), sort_comp(L, -1, -2)) {
      if (l_unlikely(i == up - 1))  /* a[i] < P  but a[up - 1] == P  ?? */
        luaL_error(L, "invalid order function for sorting");
      lua_pop(L, 1);  /* remove a[i] */
    }
    /* after the loop, a[i] >= P and a[lo .. i - 1] < P */
    /* next loop: repeat --j while P < a[j] */
    while ((void)lua_geti(L, 1, --j), sort_comp(L, -3, -1)) {
      if (l_unlikely(j < i))  /* j < i  but  a[j] > P ?? */
        luaL_error(L, "invalid order function for sorting");
      lua_pop(L, 1);  /* remove a[j] */
    }
    /* after the loop, a[j] <= P and a[j + 1 .. up] >= P */
    if (j < i) {  /* no elements out of place? */
      /* a[lo .. i - 1] <= P <= a[j + 1 .. i .. up] */
      lua_pop(L, 1);  /* pop a[j] */
      /* swap pivot (a[up - 1]) with a[i] to satisfy pos-condition */
      set2(L, up - 1, i);
      return i;
    }
    /* otherwise, swap a[i] - a[j] to restore invariant and repeat */
    set2(L, i, j);
  }
}


/*
** Choose an element in the middle (2nd-3th quarters) of [lo,up]
** "randomized" by 'rnd'
*/
static IdxT choosePivot (IdxT lo, IdxT up, unsigned int rnd) {
  IdxT r4 = (up - lo) / 4;  /* range/4 */
  IdxT p = rnd % (r4 * 2) + (lo + r4);
  lua_assert(lo + r4 <= p && p <= up - r4);
  return p;
}


/*
** Quicksort algorithm (recursive function)
*/
static void auxsort (lua_State *L, IdxT lo, IdxT up,
                                   unsigned int rnd) {
  while (lo < up) {  /* loop for tail recursion */
    IdxT p;  /* Pivot index */
    IdxT n;  /* to be used later */
    /* sort elements 'lo', 'p', and 'up' */
    lua_geti(L, 1, lo);
    lua_geti(L, 1, up);
    if (sort_comp(L, -1, -2))  /* a[up] < a[lo]? */
      set2(L, lo, up);  /* swap a[lo] - a[up] */
    else
      lua_pop(L, 2);  /* remove both values */
    if (up - lo == 1)  /* only 2 elements? */
      return;  /* already sorted */
    if (up - lo < RANLIMIT || rnd == 0)  /* small interval or no randomize? */
      p = (lo + up)/2;  /* middle element is a good pivot */
    else  /* for larger intervals, it is worth a random pivot */
      p = choosePivot(lo, up, rnd);
    lua_geti(L, 1, p);
    lua_geti(L, 1, lo);
    if (sort_comp(L, -2, -1))  /* a[p] < a[lo]? */
      set2(L, p, lo);  /* swap a[p] - a[lo] */
    else {
      lua_pop(L, 1);  /* remove a[lo] */
      lua_geti(L, 1, up);
      if (sort_comp(L, -1, -2))  /* a[up] < a[p]? */
        set2(L, p, up);  /* swap a[up] - a[p] */
      else
        lua_pop(L, 2);
    }
    if (up - lo == 2)  /* only 3 elements? */
      return;  /* already sorted */
    lua_geti(L, 1, p);  /* get middle element (Pivot) */
    lua_pushvalue(L, -1);  /* push Pivot */
    lua_geti(L, 1, up - 1);  /* push a[up - 1] */
    set2(L, p, up - 1);  /* swap Pivot (a[p]) with a[up - 1] */
    p = partition(L, lo, up);
    /* a[lo .. p - 1] <= a[p] == P <= a[p + 1 .. up] */
    if (p - lo < up - p) {  /* lower interval is smaller? */
      auxsort(L, lo, p - 1, rnd);  /* call recursively for lower interval */
      n = p - lo;  /* size of smaller interval */
      lo = p + 1;  /* tail call for [p + 1 .. up] (upper interval) */
    }
    else {
      auxsort(L, p + 1, up, rnd);  /* call recursively for upper interval */
      n = up - p;  /* size of smaller interval */
      up = p - 1;  /* tail call for [lo .. p - 1]  (lower interval) */
    }
    if ((up - lo) / 128 > n) /* partition too imbalanced? */
      rnd = l_randomizePivot();  /* try a new randomization */
  }  /* tail call auxsort(L, lo, up, rnd) */
}


/*
** For each key-value pair in the table at -1, assigns it to the table at -2.
** Pops the latter table from the stack.
*/
static void trivialcopy (lua_State* L) {
  lua_pushnil(L);
  /* stack now: newtable, table, key */
  while (lua_next(L, -2)) {
    /* stack now: newtable, table, key, value */
    lua_pushvalue(L, -2);
    lua_pushvalue(L, -2);
    lua_settable(L, -6);
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}


template <bool make_copy>
static int sort (lua_State *L) {
  if (make_copy) {
    lua_newtable(L);
    lua_pushvalue(L, 1);
    trivialcopy(L);
    lua_replace(L, 1);
  }
  lua_Integer n = aux_getn(L, 1, TAB_RW);
  if (n > 1) {  /* non-trivial interval? */
    luaL_argcheck(L, n < INT_MAX, 1, "array too big");
    if (!lua_isnoneornil(L, 2))  /* is there a 2nd argument? */
      luaL_checktype(L, 2, LUA_TFUNCTION);  /* must be a function */
    lua_settop(L, 2);  /* make sure there are two arguments */
    auxsort(L, 1, (IdxT)n, 0);
  }
  lua_settop(L, 1);
  return 1;
}


static int getn (lua_State *L) {
  lua_pushinteger(L, aux_getn(L, 1, TAB_RW));
  return 1;
}


#ifndef PLUTO_DISABLE_TABLE_FREEZING
static int tfreeze(lua_State* L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  if (lua_gettop(L) > 1) {
    luaL_error(L, "more arguments than expected to table.freeze");
  }
  else {
    lua_freezetable(L, 1);
  }
  return 1;
}


static int tisfrozen (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushboolean(L, lua_istablefrozen(L, 1));
  return 1;
}
#endif


static int tcontains (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);

  lua_pushvalue(L, 1);
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    if (lua_compare(L, 2, -2, LUA_OPEQ)) {
      lua_pushvalue(L, -1);
      return 1;
    }
    else {
      lua_pop(L, 2);
    }
  }
  
  lua_pop(L, 1);
  lua_pushnil(L);
  return 1;
}


template <bool make_copy>
static int tfilter (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  const bool callwithkey = lua_istrue(L, 3);

  if (make_copy)
    lua_newtable(L);
  lua_pushvalue(L, 1);
  if (make_copy) {
    trivialcopy(L);
  }
  lua_pushnil(L);
  /* stack now: table, key */
  while (lua_next(L, -2)) {
    /* stack now: table, key, value */
    lua_pushvalue(L, 2);
    if (callwithkey) {
      lua_pushvalue(L, -3);
      lua_pushvalue(L, -3);
      lua_call(L, 2, 1);
    }
    else {
      lua_pushvalue(L, -2);
      lua_call(L, 1, 1);
    }
    /* stack now: table, key, value, bKeep */

    const bool bKeep = lua_toboolean(L, -1);
    lua_pop(L, 1);
    /* stack now: table, key, value */
    if (!bKeep) {
      lua_pushvalue(L, -2);
      /* stack now: table, key, value, key */
      lua_pushnil(L);
      /* stack now: table, key, value, key, value */
      lua_settable(L, -5);
      /* stack now: table, key, value */
    }

    lua_pop(L, 1);
    /* stack now: table, key */
  }
  /* stack now: table */
  return 1;
}


template <bool make_copy>
static int tmap (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  const bool callwithkey = lua_istrue(L, 3);

  if (make_copy)
    lua_newtable(L);
  lua_pushvalue(L, 1);
  lua_pushnil(L);
  /* stack now: table, key */
  while (lua_next(L, -2)) {
    /* stack now: table, key, value */
    lua_pushvalue(L, 2);
    if (callwithkey) {
      lua_pushvalue(L, -3);
      lua_pushvalue(L, -3);
      lua_call(L, 2, 1);
    }
    else {
      lua_pushvalue(L, -2);
      lua_call(L, 1, 1);
    }
    /* stack now: table, key, value, mapped_value */
    lua_pushvalue(L, -3);
    lua_pushvalue(L, -2);
    /* stack now: table, key, value, mapped_value, key, mapped_value */
    lua_settable(L, make_copy ? -7 : -6);
    /* stack now: table, key, value, mapped_value */
    lua_pop(L, 2);
    /* stack now: table, key */
  }
  /* stack now: table */
  if (make_copy)
    lua_pop(L, 1);
  return 1;
}


template <bool make_copy>
static int treverse (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  if (make_copy) {
    lua_settop(L, 1);
    lua_newtable(L);
  }
  const lua_Unsigned l = lua_rawlen(L, 1);
  for (lua_Unsigned i = 1; i <= l/2; ++i) {
    lua_pushinteger(L, l - i + 1);
    lua_pushinteger(L, i);
    lua_rawget(L, 1);
    lua_pushinteger(L, i);
    lua_pushinteger(L, l - i + 1);
    lua_rawget(L, 1);
    lua_rawset(L, make_copy ? 2 : 1);
    lua_rawset(L, make_copy ? 2 : 1);
  }
  if (make_copy && (l % 2 != 0)) {
    /* for oddly sized tables, we need to manually copy the element in the middle */
    lua_pushinteger(L, l/2+1);
    lua_pushinteger(L, l/2+1);
    lua_rawget(L, 1);
    lua_rawset(L, 2);
  }
  return 1;
}


TValue *index2value (lua_State *L, int idx);
template <bool make_copy>
static int treorder (lua_State* L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 1);

  if (make_copy)
    lua_newtable(L);

  lua_pushvalue(L, 1); // stack: table
  lua_pushnil(L); // stack: table, key

  lua_Integer idx = 1;
  while (lua_next(L, -2)) { // stack: table, key, value
    if (lua_isinteger(L, -2)) {
      if (!make_copy) {
        if (auto val = lua_tointeger(L, -2); val > idx) {
          lua_pushinteger(L, val); // stack: table, key, value, key
          lua_pushnil(L); // stack: table, key, value, key, value
          lua_settable(L, 1); // stack: table, key, value
        }
      }

      lua_pushinteger(L, idx++); // stack: table, key, value, key
      lua_pushvalue(L, -2); // stack; table, key, value, key, value
      lua_settable(L, make_copy ? 2 : 1); // stack; table, key, value
    }

    lua_pop(L, 1); // stack: table, key
  }
  if (make_copy)
    lua_pop(L, 1);

  luaH_resizearray(L, hvalue(index2value(L, -1)), (unsigned int)idx);
  return 1;
}


static int tsize (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  Table *t = hvalue(index2value(L, 1));

  const bool hashonly = lua_istrue(L, 2);

  unsigned int size = luaH_gethsize(t);
  if (!hashonly)
    size += luaH_realasize(t);

  lua_pushinteger(L, size);
  return 1;
}


static int treduce (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  if (!lua_isnoneornil(L, 3)) {
    lua_pushvalue(L, 3);
  }
  else lua_pushinteger(L, 0);

  lua_pushvalue(L, 1);
  lua_pushnil(L);
  /* stack now: accum, table, key */
  while (lua_next(L, -2)) {
    /* stack now: accum, table, key, value */
    lua_pushvalue(L, 2);
    lua_pushvalue(L, -5);
    /* stack now: accum, table, key, value, func, accum */
    lua_pushvalue(L, -3);
    /* stack now: accum, table, key, value, func, accum, value */
    lua_call(L, 2, 1);
    /* stack now: accum, table, key, value, new_accum */
    lua_replace(L, -5);
    lua_pop(L, 1);
    /* stack now: accum, table, key */
  }
  lua_pop(L, 1);
  return 1;
}


static int tfind (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  lua_pushvalue(L, 1);
  lua_pushnil(L);
  /* stack now: table, key */
  while (lua_next(L, -2)) {
    /* stack now: table, key, value */
    lua_pushvalue(L, 2);
    /* stack now: table, key, value, func */
    lua_pushvalue(L, -2);
    /* stack now: table, key, value, func, value */
    lua_call(L, 1, 1);
    /* stack now: table, key, value, bool */
    if (lua_istrue(L, -1)) {
      lua_pop(L, 1);
      return 1;
    }
    lua_pop(L, 2);
    /* stack now: table, key */
  }
  lua_pop(L, 1);
  lua_pushnil(L);
  return 1;
}


static int checkall (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);

  lua_pushvalue(L, 1);
  lua_pushnil(L);
  /* stack now: table, key */
  while (lua_next(L, -2)) {
    /* stack now: table, key, value */
    lua_pushvalue(L, 2);
    /* stack now: table, key, value, func */
    lua_pushvalue(L, -2);
    /* stack now: table, key, value, func, value */
    lua_call(L, 1, 1);
    /* stack now: table, key, value, bool */
    if (!lua_istrue(L, -1)) {
      return 1;
    }
    lua_pop(L, 2);
    /* stack now: table, key */
  }
  lua_pop(L, 1);
  lua_pushboolean(L, true);
  return 1;
}


static int tclear (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaH_clear(L, hvalue(index2value(L, 1)));
  return 0;
}


/* }====================================================== */


static const luaL_Reg tab_funcs[] = {
  {"clear", tclear},
  {"checkall", checkall},
  {"find", tfind},
  {"reduce", treduce},
  {"size", tsize},
  {"reorder", treorder<false>},
  {"reordered", treorder<true>},
  {"reverse", treverse<false>},
  {"reversed", treverse<true>},
  {"map", tmap<false>},
  {"mapped", tmap<true>},
  {"filter", tfilter<false>},
  {"filtered", tfilter<true>},
  {"foreach", foreach},
  {"contains", tcontains},
#ifndef PLUTO_DISABLE_TABLE_FREEZING
  {"isfrozen", tisfrozen},
  {"freeze", tfreeze},
#endif
  {"concat", tconcat},
  {"insert", tinsert},
  {"pack", tpack},
  {"unpack", tunpack},
  {"remove", tremove},
  {"move", tmove},
  {"sort", sort<false>},
  {"sorted", sort<true>},
  {"getn", getn},
  {NULL, NULL}
};


LUAMOD_API int luaopen_table (lua_State *L) {
  luaL_newlib(L, tab_funcs);

  lua_pushliteral(L, "min");
  luaL_loadstring(L, "return |t| -> table.reduce(t, math.min, math.maxinteger)");
  lua_call(L, 0, 1);
  lua_settable(L, -3);

  lua_pushliteral(L, "max");
  luaL_loadstring(L, "return |t| -> table.reduce(t, math.max, math.mininteger)");
  lua_call(L, 0, 1);
  lua_settable(L, -3);

  return 1;
}

