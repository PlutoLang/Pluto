/*
** $Id: liolib.c $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/

#define liolib_c
#define LUA_LIB

#include "lprefix.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <filesystem>
#include <fstream>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include "vendor/Soup/soup/filesystem.hpp"
#include "vendor/Soup/soup/string.hpp"
#include "vendor/Soup/soup/unicode.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#endif


// As a "last stand" measure to ensure that Pluto, when compiled as a DLL, can itself not be loaded via package.loadlib to provide functions that the integrator wanted disabled.
// This should be reasonably effective on Windows where the DLL is expected to be shipped with the program, if static linking is not used.
// On Linux, static linking is the way to go -- and even then, it is still possible that there is a SO with these functions fully functional somewhere on the system.
#ifdef PLUTO_NO_FILESYSTEM
#define FS_FUNCTION return 0;
#else
#define FS_FUNCTION
#endif


/*
** Change this macro to accept other modes for 'fopen' besides
** the standard ones.
*/
#if !defined(l_checkmode)

/* accepted extensions to 'mode' in 'fopen' */
#if !defined(L_MODEEXT)
#define L_MODEEXT	"b"
#endif

/* Check whether 'mode' matches '[rwa]%+?[L_MODEEXT]*' */
static int l_checkmode (const char *mode) {
  return (*mode != '\0' && strchr("rwa", *(mode++)) != NULL &&
         (*mode != '+' || ((void)(++mode), 1)) &&  /* skip if char is '+' */
         (strspn(mode, L_MODEEXT) == strlen(mode)));  /* check extensions */
}

#endif

/*
** {======================================================
** l_popen spawns a new process connected to the current
** one through the file streams.
** =======================================================
*/

#if !defined(l_popen)		/* { */

#if defined(LUA_USE_POSIX)	/* { */

#define l_popen(L,c,m)		(fflush(NULL), popen(c,m))
#define l_pclose(L,file)	(pclose(file))

#elif defined(LUA_USE_WINDOWS)	/* }{ */

#define l_popen(L,c,m)		(_popen(c,m))
#define l_pclose(L,file)	(_pclose(file))

#if !defined(l_checkmodep)
/* Windows accepts "[rw][bt]?" as valid modes */
#define l_checkmodep(m)	((m[0] == 'r' || m[0] == 'w') && \
  (m[1] == '\0' || ((m[1] == 'b' || m[1] == 't') && m[2] == '\0')))
#endif

#else				/* }{ */

/* ISO C definitions */
#define l_popen(L,c,m)  \
      ((void)c, (void)m, \
      luaL_error(L, "'popen' not supported"), \
      (FILE*)0)
#define l_pclose(L,file)		((void)L, (void)file, -1)

#endif				/* } */

#endif				/* } */


#if !defined(l_checkmodep)
/* By default, Lua accepts only "r" or "w" as valid modes */
#define l_checkmodep(m)        ((m[0] == 'r' || m[0] == 'w') && m[1] == '\0')
#endif

/* }====================================================== */


#if !defined(l_getc)		/* { */

#if defined(LUA_USE_POSIX)
#define l_getc(f)		getc_unlocked(f)
#define l_lockfile(f)		flockfile(f)
#define l_unlockfile(f)		funlockfile(f)
#else
#define l_getc(f)		getc(f)
#define l_lockfile(f)		((void)0)
#define l_unlockfile(f)		((void)0)
#endif

#endif				/* } */


/*
** {======================================================
** l_fseek: configuration for longer offsets
** =======================================================
*/

#if !defined(l_fseek)		/* { */

#if defined(LUA_USE_POSIX)	/* { */

#include <sys/types.h>

#define l_fseek(f,o,w)		fseeko(f,o,w)
#define l_ftell(f)		ftello(f)
#define l_seeknum		off_t

#elif defined(LUA_USE_WINDOWS) && !defined(_CRTIMP_TYPEINFO) \
   && defined(_MSC_VER) && (_MSC_VER >= 1400)	/* }{ */

/* Windows (but not DDK) and Visual C++ 2005 or higher */
#define l_fseek(f,o,w)		_fseeki64(f,o,w)
#define l_ftell(f)		_ftelli64(f)
#define l_seeknum		__int64

#else				/* }{ */

/* ISO C definitions */
#define l_fseek(f,o,w)		fseek(f,o,w)
#define l_ftell(f)		ftell(f)
#define l_seeknum		long

#endif				/* } */

#endif				/* } */

/* }====================================================== */



#define IO_PREFIX	"_IO_"
#define IOPREF_LEN	(sizeof(IO_PREFIX)/sizeof(char) - 1)
#define IO_INPUT	(IO_PREFIX "input")
#define IO_OUTPUT	(IO_PREFIX "output")


typedef luaL_Stream LStream;


#define tolstream(L)	((LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE))

#define isclosed(p)	((p)->closef == NULL)


static int io_type (lua_State *L) {
  FS_FUNCTION
  LStream *p;
  luaL_checkany(L, 1);
  p = (LStream *)luaL_testudata(L, 1, LUA_FILEHANDLE);
  if (p == NULL)
    luaL_pushfail(L);  /* not a file */
  else if (isclosed(p))
    lua_pushliteral(L, "closed file");
  else
    lua_pushliteral(L, "file");
  return 1;
}


static int f_tostring (lua_State *L) {
  LStream *p = tolstream(L);
  if (isclosed(p))
    lua_pushliteral(L, "file (closed)");
  else
    lua_pushfstring(L, "file (%p)", p->f);
  return 1;
}


static FILE *tofile (lua_State *L) {
  LStream *p = tolstream(L);
  if (l_unlikely(isclosed(p)))
    luaL_error(L, "attempt to use a closed file");
  lua_assert(p->f);
  return p->f;
}


/*
** When creating file handles, always creates a 'closed' file handle
** before opening the actual file; so, if there is a memory error, the
** handle is in a consistent state.
*/
static LStream *newprefile (lua_State *L) {
  LStream *p = (LStream *)lua_newuserdatauv(L, sizeof(LStream), 0);
  p->closef = NULL;  /* mark file handle as 'closed' */
  luaL_setmetatable(L, LUA_FILEHANDLE);
  return p;
}


/*
** Calls the 'close' function from a file handle. The 'volatile' avoids
** a bug in some versions of the Clang compiler (e.g., clang 3.0 for
** 32 bits).
*/
static int aux_close (lua_State *L) {
  FS_FUNCTION
  LStream *p = tolstream(L);
  volatile lua_CFunction cf = p->closef;
  p->closef = NULL;  /* mark stream as closed */
  return (*cf)(L);  /* close it */
}


static int f_close (lua_State *L) {
  FS_FUNCTION
  tofile(L);  /* make sure argument is an open stream */
  return aux_close(L);
}


static int io_close (lua_State *L) {
  FS_FUNCTION
  if (lua_isnone(L, 1))  /* no argument? */
    lua_getfield(L, LUA_REGISTRYINDEX, IO_OUTPUT);  /* use default output */
  return f_close(L);
}


static int f_gc (lua_State *L) {
  FS_FUNCTION
  LStream *p = tolstream(L);
  if (!isclosed(p) && p->f != NULL)
    aux_close(L);  /* ignore closed and incompletely open files */
  return 0;
}


/*
** function to close regular files
*/
static int io_fclose (lua_State *L) {
  FS_FUNCTION
  LStream *p = tolstream(L);
  errno = 0;
  return luaL_fileresult(L, (fclose(p->f) == 0), NULL);
}


static LStream *newfile (lua_State *L) {
  FS_FUNCTION
  LStream *p = newprefile(L);
  p->f = NULL;
  p->closef = &io_fclose;
  return p;
}


#ifdef PLUTO_READ_FILE_HOOK
extern bool PLUTO_READ_FILE_HOOK(lua_State* L, const char* path);
#endif

#ifdef PLUTO_WRITE_FILE_HOOK
extern bool PLUTO_WRITE_FILE_HOOK(lua_State* L, const char* path);
#endif


static void policycheck (lua_State *L, const char *filename, const char *mode) {
  if (mode[0] == 'r') {
#ifdef PLUTO_READ_FILE_HOOK
    if (!PLUTO_READ_FILE_HOOK(L, filename)) {
      luaL_error(L, "disallowed by content moderation policy");
    }
#endif
  }
  else {
#ifdef PLUTO_WRITE_FILE_HOOK
    if (!PLUTO_WRITE_FILE_HOOK(L, (const char*)filename)) {
      luaL_error(L, "disallowed by content moderation policy");
    }
#endif
  }
}


static void opencheck (lua_State *L, const char *fname, const char *mode) {
  LStream *p = newfile(L);
  policycheck(L, fname, mode);
  p->f = luaL_fopen(fname, strlen(fname), mode, strlen(mode));
  if (l_unlikely(p->f == NULL))
    luaL_error(L, "cannot open file '%s' (%s)", fname, strerror(errno));
}


static int io_open (lua_State *L) {
  FS_FUNCTION
  size_t filename_len, mode_len;
  const char *filename = luaL_checklstring(L, 1, &filename_len);
  const char *mode = luaL_optlstring(L, 2, "r", &mode_len);
  LStream *p = newfile(L);
  luaL_argcheck(L, l_checkmode(mode), 2, "invalid mode");
  policycheck(L, filename, mode);
  errno = 0;
  p->f = luaL_fopen(filename, filename_len, mode, mode_len);

  if (p->f != nullptr)
  {
    lua_getmetatable(L, -1);
    lua_pushstring(L, filename);
    lua_setfield(L, -2, "__path");
    lua_pop(L, 1);
    return 1;
  }
  else
  {
    return luaL_fileresult(L, 0, filename);
  }
}


/*
** function to close 'popen' files
*/
static int io_pclose (lua_State *L) {
  FS_FUNCTION
  LStream *p = tolstream(L);
  errno = 0;
  return luaL_execresult(L, l_pclose(L, p->f));
}


static int io_popen (lua_State *L) {
#ifdef PLUTO_NO_OS_EXECUTE
  return 0;
#endif
  FS_FUNCTION
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  LStream *p = newprefile(L);
  luaL_argcheck(L, l_checkmodep(mode), 2, "invalid mode");
  errno = 0;
  p->f = l_popen(L, filename, mode);
  p->closef = &io_pclose;

  if (p->f != nullptr)
  {
    lua_getmetatable(L, -1);
    lua_pushstring(L, filename);
    lua_setfield(L, -2, "__path");
    lua_pop(L, 1);
    return 1;
  }
  else
  {
    return luaL_fileresult(L, 0, filename);
  }
}


static int io_tmpfile (lua_State *L) {
  FS_FUNCTION
  LStream *p = newfile(L);
  errno = 0;
  p->f = tmpfile();
  return (p->f == NULL) ? luaL_fileresult(L, 0, NULL) : 1;
}


static FILE *getiofile (lua_State *L, const char *findex) {
  LStream *p;
  lua_getfield(L, LUA_REGISTRYINDEX, findex);
  p = (LStream *)lua_touserdata(L, -1);
  if (l_unlikely(isclosed(p)))
    luaL_error(L, "default %s file is closed", findex + IOPREF_LEN);
  return p->f;
}


static int g_iofile (lua_State *L, const char *f, const char *mode) {
  if (!lua_isnoneornil(L, 1)) {
    const char *filename = lua_tostring(L, 1);
    if (filename)
      opencheck(L, filename, mode);
    else {
      tofile(L);  /* check that it's a valid file handle */
      lua_pushvalue(L, 1);
    }
    lua_setfield(L, LUA_REGISTRYINDEX, f);
  }
  /* return current value */
  lua_getfield(L, LUA_REGISTRYINDEX, f);
  return 1;
}


static int io_input (lua_State *L) {
  FS_FUNCTION
  return g_iofile(L, IO_INPUT, "r");
}


static int io_output (lua_State *L) {
  FS_FUNCTION
  return g_iofile(L, IO_OUTPUT, "w");
}


static int io_readline (lua_State *L);


/*
** maximum number of arguments to 'f:lines'/'io.lines' (it + 3 must fit
** in the limit for upvalues of a closure)
*/
#define MAXARGLINE	250

/*
** Auxiliary function to create the iteration function for 'lines'.
** The iteration function is a closure over 'io_readline', with
** the following upvalues:
** 1) The file being read (first value in the stack)
** 2) the number of arguments to read
** 3) a boolean, true iff file has to be closed when finished ('toclose')
** *) a variable number of format arguments (rest of the stack)
*/
static void aux_lines (lua_State *L, int toclose) {
  int n = lua_gettop(L) - 1;  /* number of arguments to read */
  luaL_argcheck(L, n <= MAXARGLINE, MAXARGLINE + 2, "too many arguments");
  lua_pushvalue(L, 1);  /* file */
  lua_pushinteger(L, n);  /* number of arguments to read */
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  lua_rotate(L, 2, 3);  /* move the three values to their positions */
  lua_pushcclosure(L, io_readline, 3 + n);
}


static int f_lines (lua_State *L) {
  FS_FUNCTION
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 0);
  return 1;
}


/*
** Return an iteration function for 'io.lines'. If file has to be
** closed, also returns the file itself as a second result (to be
** closed as the state at the exit of a generic for).
*/
static int io_lines (lua_State *L) {
  FS_FUNCTION
  int toclose;
  if (lua_isnone(L, 1)) lua_pushnil(L);  /* at least one argument */
  if (lua_isnil(L, 1)) {  /* no file name? */
    lua_getfield(L, LUA_REGISTRYINDEX, IO_INPUT);  /* get default input */
    lua_replace(L, 1);  /* put it at index 1 */
    tofile(L);  /* check that it's a valid file handle */
    toclose = 0;  /* do not close it after iteration */
  }
  else {  /* open a new file */
    const char *filename = luaL_checkstring(L, 1);
    opencheck(L, filename, "r");
    lua_replace(L, 1);  /* put file at index 1 */
    toclose = 1;  /* close it after iteration */
  }
  aux_lines(L, toclose);  /* push iteration function */
  if (toclose) {
    lua_pushnil(L);  /* state */
    lua_pushnil(L);  /* control */
    lua_pushvalue(L, 1);  /* file is the to-be-closed variable (4th result) */
    return 4;
  }
  else
    return 1;
}


/*
** {======================================================
** READ
** =======================================================
*/


/* maximum length of a numeral */
#if !defined (L_MAXLENNUM)
#define L_MAXLENNUM     200
#endif


/* auxiliary structure used by 'read_number' */
typedef struct {
  FILE *f;  /* file being read */
  int c;  /* current character (look ahead) */
  int n;  /* number of elements in buffer 'buff' */
  char buff[L_MAXLENNUM + 1];  /* +1 for ending '\0' */
} RN;


/*
** Add current char to buffer (if not out of space) and read next one
*/
static int nextc (RN *rn) {
  if (l_unlikely(rn->n >= L_MAXLENNUM)) {  /* buffer overflow? */
    rn->buff[0] = '\0';  /* invalidate result */
    return 0;  /* fail */
  }
  else {
    rn->buff[rn->n++] = rn->c;  /* save current char */
    rn->c = l_getc(rn->f);  /* read next one */
    return 1;
  }
}


/*
** Accept current char if it is in 'set' (of size 2)
*/
static int test2 (RN *rn, const char *set) {
  if (rn->c == set[0] || rn->c == set[1])
    return nextc(rn);
  else return 0;
}


/*
** Read a sequence of (hex)digits
*/
static int readdigits (RN *rn, int hex) {
  int count = 0;
  while ((hex ? isxdigit(rn->c) : isdigit(rn->c)) && nextc(rn))
    count++;
  return count;
}


/*
** Read a number: first reads a valid prefix of a numeral into a buffer.
** Then it calls 'lua_stringtonumber' to check whether the format is
** correct and to convert it to a Lua number.
*/
static int read_number (lua_State *L, FILE *f) {
  RN rn;
  int count = 0;
  int hex = 0;
  char decp[2];
  rn.f = f; rn.n = 0;
  decp[0] = lua_getlocaledecpoint();  /* get decimal point from locale */
  decp[1] = '.';  /* always accept a dot */
  l_lockfile(rn.f);
  do { rn.c = l_getc(rn.f); } while (isspace(rn.c));  /* skip spaces */
  test2(&rn, "-+");  /* optional sign */
  if (test2(&rn, "00")) {
    if (test2(&rn, "xX")) hex = 1;  /* numeral is hexadecimal */
    else count = 1;  /* count initial '0' as a valid digit */
  }
  count += readdigits(&rn, hex);  /* integral part */
  if (test2(&rn, decp))  /* decimal point? */
    count += readdigits(&rn, hex);  /* fractional part */
  if (count > 0 && test2(&rn, (hex ? "pP" : "eE"))) {  /* exponent mark? */
    test2(&rn, "-+");  /* exponent sign */
    readdigits(&rn, 0);  /* exponent digits */
  }
  ungetc(rn.c, rn.f);  /* unread look-ahead char */
  l_unlockfile(rn.f);
  rn.buff[rn.n] = '\0';  /* finish string */
  if (l_likely(lua_stringtonumber(L, rn.buff)))
    return 1;  /* ok, it is a valid number */
  else {  /* invalid format */
   lua_pushnil(L);  /* "result" to be removed */
   return 0;  /* read fails */
  }
}


static int test_eof (lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);  /* no-op when c == EOF */
  lua_pushliteral(L, "");
  return (c != EOF);
}


static int read_line (lua_State *L, FILE *f, int chop) {
  luaL_Buffer b;
  int c;
  luaL_buffinit(L, &b);
  do {  /* may need to read several chunks to get whole line */
    char *buff = luaL_prepbuffer(&b);  /* preallocate buffer space */
    int i = 0;
    l_lockfile(f);  /* no memory errors can happen inside the lock */
    while (i < LUAL_BUFFERSIZE && (c = l_getc(f)) != EOF && c != '\n')
      buff[i++] = c;  /* read up to end of line or buffer limit */
    l_unlockfile(f);
    luaL_addsize(&b, i);
  } while (c != EOF && c != '\n');  /* repeat until end of line */
  if (!chop && c == '\n')  /* want a newline and have one? */
    luaL_addchar(&b, c);  /* add ending newline to result */
  luaL_pushresult(&b);  /* close buffer */
  /* return ok if read something (either a newline or something else) */
  return (c == '\n' || lua_rawlen(L, -1) > 0);
}


static void read_all (lua_State *L, FILE *f) {
  size_t nr;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  do {  /* read file in chunks of LUAL_BUFFERSIZE bytes */
    char *p = luaL_prepbuffer(&b);
    nr = fread(p, sizeof(char), LUAL_BUFFERSIZE, f);
    luaL_addsize(&b, nr);
  } while (nr == LUAL_BUFFERSIZE);
  luaL_pushresult(&b);  /* close buffer */
}


static int read_chars (lua_State *L, FILE *f, size_t n) {
  size_t nr;  /* number of chars actually read */
  char *p;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  p = luaL_prepbuffsize(&b, n);  /* prepare buffer to read whole block */
  nr = fread(p, sizeof(char), n, f);  /* try to read 'n' chars */
  luaL_addsize(&b, nr);
  luaL_pushresult(&b);  /* close buffer */
  return (nr > 0);  /* true iff read something */
}


static int g_read (lua_State *L, FILE *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int n, success;
  clearerr(f);
  errno = 0;
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f, 1);
    n = first + 1;  /* to return 1 result */
  }
  else {
    /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)luaL_checkinteger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = luaL_checkstring(L, n);
        if (*p == '*') p++;  /* skip optional '*' (for compatibility) */
        switch (*p) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f, 1);
            break;
          case 'L':  /* line with end-of-line */
            success = read_line(L, f, 0);
            break;
          case 'a':  /* file */
            read_all(L, f);  /* read entire file */
            success = 1; /* always success */
            break;
          default:
            luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return luaL_fileresult(L, 0, NULL);
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    luaL_pushfail(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (lua_State *L) {
  FS_FUNCTION
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static int f_read (lua_State *L) {
  FS_FUNCTION
  return g_read(L, tofile(L), 2);
}


/*
** Iteration function for 'lines'.
*/
static int io_readline (lua_State *L) {
  FS_FUNCTION
  LStream *p = (LStream *)lua_touserdata(L, lua_upvalueindex(1));
  int i;
  int n = (int)lua_tointeger(L, lua_upvalueindex(2));
  if (isclosed(p))  /* file is already closed? */
    luaL_error(L, "file is already closed");
  lua_settop(L , 1);
  luaL_checkstack(L, n, "too many arguments");
  for (i = 1; i <= n; i++)  /* push arguments to 'g_read' */
    lua_pushvalue(L, lua_upvalueindex(3 + i));
  n = g_read(L, p->f, 2);  /* 'n' is number of results */
  lua_assert(n > 0);  /* should return at least a nil */
  if (lua_toboolean(L, -n))  /* read at least one value? */
    return n;  /* return them */
  else {  /* first result is false: EOF or error */
    if (n > 1) {  /* is there error information? */
      /* 2nd result is error message */
      luaL_error(L, "%s", lua_tostring(L, -n + 1));
    }
    if (lua_toboolean(L, lua_upvalueindex(3))) {  /* generator created file? */
      lua_settop(L, 0);  /* clear stack */
      lua_pushvalue(L, lua_upvalueindex(1));  /* push file at index 1 */
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (lua_State *L, FILE *f, int arg) {
  int nargs = lua_gettop(L) - arg;
  int status = 1;
  errno = 0;
  for (; nargs--; arg++) {
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      int len = lua_isinteger(L, arg)
                ? fprintf(f, LUA_INTEGER_FMT,
                             (LUAI_UACINT)lua_tointeger(L, arg))
                : fprintf(f, LUA_NUMBER_FMT,
                             (LUAI_UACNUMBER)lua_tonumber(L, arg));
      status = status && (len > 0);
    }
    else {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  if (l_likely(status))
    return 1;  /* file handle already on stack top */
  else
    return luaL_fileresult(L, status, NULL);
}


static int io_write (lua_State *L) {
  FS_FUNCTION
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static int f_write (lua_State *L) {
  FS_FUNCTION
  FILE *f = tofile(L);
  lua_pushvalue(L, 1);  /* push file at the stack top (to be returned) */
  return g_write(L, f, 2);
}


static int f_seek (lua_State *L) {
  FS_FUNCTION
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = luaL_checkoption(L, 2, "cur", modenames);
  lua_Integer p3 = luaL_optinteger(L, 3, 0);
  l_seeknum offset = (l_seeknum)p3;
  luaL_argcheck(L, (lua_Integer)offset == p3, 3,
                  "not an integer in proper range");
  errno = 0;
  op = l_fseek(f, offset, mode[op]);
  if (l_unlikely(op))
    return luaL_fileresult(L, 0, NULL);  /* error */
  else {
    lua_pushinteger(L, (lua_Integer)l_ftell(f));
    return 1;
  }
}


static int f_setvbuf (lua_State *L) {
  FS_FUNCTION
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = luaL_checkoption(L, 2, NULL, modenames);
  lua_Integer sz = luaL_optinteger(L, 3, LUAL_BUFFERSIZE);
  int res;
  errno = 0;
  res = setvbuf(f, NULL, mode[op], (size_t)sz);
  return luaL_fileresult(L, res == 0, NULL);
}



static int io_flush (lua_State *L) {
  FS_FUNCTION
  FILE *f = getiofile(L, IO_INPUT);
  errno = 0;
  return luaL_fileresult(L, fflush(f) == 0, NULL);
}


static int f_flush (lua_State *L) {
  FS_FUNCTION
  FILE *f = tofile(L);
  errno = 0;
  return luaL_fileresult(L, fflush(f) == 0, NULL);
}


[[nodiscard]] static std::filesystem::path& getStringStreamPathRaw(lua_State *L, int idx) {
  const char* f;

  if (lua_isuserdata(L, idx)) {
    lua_getmetatable(L, idx);
    lua_getfield(L, -1, "__path");
    if ((f = lua_tostring(L, -1)) == nullptr) {
      luaL_error(L, "cannot find path attribute of file stream (this stream is a temporary file).");
    }
  }
  else {
    f = luaL_checkstring(L, idx);
  }

#if SOUP_CPP20
  return *pluto_newclassinst(L, std::filesystem::path, soup::string::toUtf8Type(f));
#else
  return *pluto_newclassinst(L, std::filesystem::path, std::filesystem::u8path(f));
#endif
}

[[nodiscard]] static std::filesystem::path& getStringStreamPathForRead(lua_State *L, int idx) {
  auto& path = getStringStreamPathRaw(L, idx);
#ifdef PLUTO_NO_FILESYSTEM
  luaL_error(L, "disallowed by content moderation policy");
#endif
#ifdef PLUTO_READ_FILE_HOOK
  if (!PLUTO_READ_FILE_HOOK(L, (const char*)path.u8string().c_str())) {
    luaL_error(L, "disallowed by content moderation policy");
  }
#endif
  return path;
}

[[nodiscard]] static std::filesystem::path& getStringStreamPathForWrite(lua_State *L, int idx) {
  auto& path = getStringStreamPathRaw(L, idx);
#ifdef PLUTO_NO_FILESYSTEM
  luaL_error(L, "disallowed by content moderation policy");
#endif
#ifdef PLUTO_WRITE_FILE_HOOK
  if (!PLUTO_WRITE_FILE_HOOK(L, (const char*)path.u8string().c_str())) {
    luaL_error(L, "disallowed by content moderation policy");
  }
#endif
  return path;
}

static int isdir (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto ret = std::filesystem::is_directory(path, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushboolean(L, ret);
  return 1;
}


static int isfile (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto ret = std::filesystem::is_regular_file(path, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushboolean(L, ret);
  return 1;
}


static int filesize (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto ret = (lua_Integer)std::filesystem::file_size(path, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushinteger(L, ret);
  return 1;
}


static int exists (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto ret = std::filesystem::exists(path, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushboolean(L, ret);
  return 1;
}


static int io_copy (lua_State *L) {
  FS_FUNCTION
  lua_settop(L, 2);
  /* stack: arg_from, arg_to */
  auto& from = getStringStreamPathForRead(L, -2);
  /* stack: arg_from, arg_to, path_from */
  auto& to = getStringStreamPathForWrite(L, -2);
  /* stack: arg_from, arg_to, path_from, path_to */

  std::error_code ec;
  const auto to_exists = std::filesystem::is_regular_file(to, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "failed to stat destination");
  }
  if (to_exists) {
    std::filesystem::remove(to, ec);
    if (l_unlikely(ec.operator bool())) {
      luaL_error(L, "destination already exists, attempted to delete but failed");
    }
  }

  std::filesystem::copy_file(from, to, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }

  return 0;
}

static int io_copyto (lua_State *L) {
  pluto_warning(L, "io.copyto is deprecated, replace the call with io.copy.");
  return io_copy(L);
}


static int absolute (lua_State *L) {
  FS_FUNCTION
  const auto bCanonical = lua_istrue(L, 2);
  auto& f = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto r = bCanonical ? std::filesystem::canonical(f, ec) : std::filesystem::absolute(f, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushstring(L, (const char*)r.u8string().c_str());
  return 1;
}


static int relative (lua_State *L) {
  FS_FUNCTION
  auto& f = getStringStreamPathForRead(L, 1);
  std::error_code ec;
  const auto r = std::filesystem::relative(f, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushstring(L, (const char*)r.u8string().c_str());
  return 1;
}


static int io_part (lua_State *L) {
  if (lua_gettop(L) == 1) {
    auto& path = getStringStreamPathRaw(L, 1);
    pluto_pushstring(L, soup::string::fixType(path.parent_path().u8string()));
    pluto_pushstring(L, soup::string::fixType(path.filename().u8string()));
    return 2;
  }
  static const char *const parts[] = {"parent", "name", nullptr};
  int part = luaL_checkoption(L, 2, nullptr, parts);
  auto& path = getStringStreamPathRaw(L, 1);
  if (part == 0)
    path = path.parent_path();
  else
    path = path.filename();
  pluto_pushstring(L, soup::string::fixType(path.u8string()));
  return 1;
}


static int makedir (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForWrite(L, 1);
  std::error_code ec;
  const auto res = std::filesystem::create_directory(path, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  lua_pushboolean(L, res);
#if SOUP_WINDOWS
  if (res) {
    if (path.filename().empty())
      path = path.parent_path();
    if (path.filename().c_str()[0] == '.') {
      SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }
  }
#endif
  return 1;
}


static int makedirs (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForWrite(L, 1);
  std::error_code ec;
  lua_pushboolean(L, std::filesystem::create_directories(path, ec));
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  return 1;
}


/*
  0.4.0 Changes:
    - Second boolean parameter will toggle recursive iteration.
    - Returns an empty table instead of throwing an error if 'f' is not a directory.
*/
static void listdir_r (lua_State* L, lua_Integer& i, const std::filesystem::path& f) {
  std::error_code ec;
  std::filesystem::directory_iterator it(f, ec);
  if (l_unlikely(ec.operator bool())) return; /* skip this directory if we failed to enter it */
  for (auto const& dir_entry : it) {
    lua_pushstring(L, (const char*)dir_entry.path().u8string().c_str());
    lua_rawseti(L, -2, ++i);
    if (dir_entry.is_directory()) {
      listdir_r(L, i, dir_entry.path());
    }
  }
}

static int listdir (lua_State *L) {
  FS_FUNCTION
  const auto recursive = lua_istrue(L, 2);
  auto& f = getStringStreamPathForRead(L, 1);
  lua_newtable(L);
  lua_Integer i = 0;
  std::error_code ec;
  std::filesystem::directory_iterator it(f, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  for (auto const& dir_entry : it) {
    lua_pushstring(L, (const char*)dir_entry.path().u8string().c_str());
    lua_rawseti(L, -2, ++i);
    if (recursive && dir_entry.is_directory()) {
      listdir_r(L, i, dir_entry.path());
    }
  }
  return 1;
}


int l_os_remove (lua_State *L) {
  FS_FUNCTION
  auto& path = getStringStreamPathForWrite(L, 1);
  std::error_code ec;
  std::filesystem::remove(path, ec);
  if (l_unlikely(ec.operator bool())) {
    lua_pushboolean(L, 0);
    lua_pushliteral(L, "operation failed");
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int l_remove (lua_State *L) {
  FS_FUNCTION
  const auto recursive = lua_istrue(L, 2);
  auto& path = getStringStreamPathForWrite(L, 1);
  std::error_code ec;
  if (recursive) {
    std::filesystem::remove_all(path, ec);
  }
  else {
    std::filesystem::remove(path, ec);
  }
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  return 0;
}


int l_os_rename (lua_State *L) {
  FS_FUNCTION
  lua_settop(L, 2);
  /* stack: arg_from, arg_to */
  auto& from = getStringStreamPathForRead(L, -2);
  /* stack: arg_from, arg_to, path_from */
  auto& to = getStringStreamPathForWrite(L, -2);
  /* stack: arg_from, arg_to, path_from, path_to */
  std::error_code ec;
  std::filesystem::rename(from, to, ec);
  if (l_unlikely(ec.operator bool())) {
    lua_pushboolean(L, 0);
    lua_pushliteral(L, "operation failed");
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}

static int l_rename (lua_State *L) {
  FS_FUNCTION
  lua_settop(L, 2);
  /* stack: arg_from, arg_to */
  auto& from = getStringStreamPathForRead(L, -2);
  /* stack: arg_from, arg_to, path_from */
  auto& to = getStringStreamPathForWrite(L, -2);
  /* stack: arg_from, arg_to, path_from, path_to */
  std::error_code ec;
  std::filesystem::rename(from, to, ec);
  if (l_unlikely(ec.operator bool())) {
    luaL_error(L, "operation failed");
  }
  return 0;
}


static int currentdir (lua_State *L) {
  FS_FUNCTION
  if (lua_gettop(L) == 0) {
    /* getter */
    auto& cd = *pluto_newclassinst(L, std::filesystem::path);
    std::error_code ec;
    cd = std::filesystem::current_path(ec);
    if (l_unlikely(ec.operator bool())) {
      luaL_error(L, "operation failed");
    }
    lua_pushstring(L, (const char*)cd.u8string().c_str());
    return 1;
  }
  else {
    /* setter */
    auto& cd = getStringStreamPathRaw(L, 1);
    std::error_code ec;
    std::filesystem::current_path(cd, ec);
    if (l_unlikely(ec.operator bool())) {
      luaL_error(L, "operation failed");
    }
    return 0;
  }
}


[[nodiscard]] static std::time_t file_time_to_unix_time (std::filesystem::file_time_type ft) {
  return std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    ).time_since_epoch()
  ).count();
}

[[nodiscard]] static std::filesystem::file_time_type unix_time_to_file_time (std::time_t ut) {
  std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(ut);
  return tp - std::chrono::system_clock::now() + std::filesystem::file_time_type::clock::now();
}

static int writetime (lua_State *L) {
  FS_FUNCTION
  if (lua_gettop(L) == 1) {
    /* getter */
    auto& file = getStringStreamPathForRead(L, 1);
    std::error_code ec;
    std::time_t ut;
    ut = file_time_to_unix_time(std::filesystem::last_write_time(file, ec));
    if (l_unlikely(ec.operator bool())) {
      luaL_error(L, "operation failed");
    }
    lua_pushinteger(L, ut);
    return 1;
  }
  else {
    /* setter */
    const auto ut = luaL_checkinteger(L, 2);
    auto& file = getStringStreamPathForWrite(L, 1);
    std::error_code ec;
    std::filesystem::last_write_time(file, unix_time_to_file_time(ut), ec);
    if (l_unlikely(ec.operator bool())) {
      luaL_error(L, "operation failed");
    }
    return 0;
  }
}


static int contents (lua_State *L) {
  FS_FUNCTION
  if (lua_gettop(L) == 1) {
    /* getter */
    auto& file = getStringStreamPathForRead(L, 1);
    size_t len;
    if (auto data = soup::filesystem::createFileMapping(file, len)) {
      lua_pushlstring(L, (const char*)data, len);
      soup::filesystem::destroyFileMapping(data, len);
      return 1;
    }
  }
  else {
    /* setter */
    size_t len;
    const char *str = luaL_checklstring(L, 2, &len);
    auto& file = getStringStreamPathForWrite(L, 1);
    std::ofstream of(file, std::ios_base::binary);
    of.write(str, len);
  }
  return 0;
}


static int io_chmod (lua_State *L) {
  FS_FUNCTION
  switch (lua_gettop(L)) {
    case 0: {  /* availability check */
      lua_pushboolean(L, !SOUP_WINDOWS);
      return 1;
    }
    case 1: {  /* getter */
#if !SOUP_WINDOWS
      auto& file = getStringStreamPathForRead(L, 1);
      struct stat st;
      if (stat(file.c_str(), &st) == 0) {
        lua_pushinteger(L, st.st_mode & 0777);
        return 1;
      }
#endif
      return 0;
    }
    case 2: {  /* setter */
#if !SOUP_WINDOWS
      const auto mode = (mode_t)luaL_checkinteger(L, 2);
      auto& file = getStringStreamPathForWrite(L, 1);
      chmod(file.c_str(), mode);
#endif
      return 0;
    }
    default: luaL_error(L, "wrong number of arguments");
  }
}


/*
** functions for 'io' library
*/
static const luaL_Reg iolib[] = {
  {"chmod", io_chmod},
  {"contents", contents},
  {"writetime", writetime},
  {"currentdir", currentdir},
  {"chdir", currentdir},
  {"cwd", currentdir},
  {"rename", l_rename},
  {"remove", l_remove},
  {"listdir", listdir},
  {"makedir", makedir},
  {"mkdir", makedir},
  {"makedirs", makedirs},
  {"mkdirs", makedirs},
  {"absolute", absolute},
  {"relative", relative},
  {"part", io_part},
  {"copy", io_copy}, /* added in Pluto 0.8.0 */
  {"copyto", io_copyto}, /* deprecated as of Pluto 0.8.0 */
  {"exists", exists},
  {"filesize", filesize},
  {"isfile", isfile},
  {"isdir", isdir},
  {"close", io_close},
  {"flush", io_flush},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"popen", io_popen},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


/*
** methods for file handles
*/
static const luaL_Reg meth[] = {
  {"read", f_read},
  {"write", f_write},
  {"lines", f_lines},
  {"flush", f_flush},
  {"seek", f_seek},
  {"close", f_close},
  {"setvbuf", f_setvbuf},
  {NULL, NULL}
};


/*
** metamethods for file handles
*/
static const luaL_Reg metameth[] = {
  {"__index", NULL},  /* placeholder */
  {"__gc", f_gc},
  {"__close", f_gc},
  {"__tostring", f_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, LUA_FILEHANDLE);  /* metatable for file handles */
  luaL_setfuncs(L, metameth, 0);  /* add metamethods to new metatable */
  luaL_newlibtable(L, meth);  /* create method table */
  luaL_setfuncs(L, meth, 0);  /* add file methods to method table */
  lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
  lua_pop(L, 1);  /* pop metatable */
}


/*
** function to (not) close the standard files stdin, stdout, and stderr
*/
static int io_noclose (lua_State *L) {
  LStream *p = tolstream(L);
  p->closef = &io_noclose;  /* keep file opened */
  luaL_pushfail(L);
  lua_pushliteral(L, "cannot close standard file");
  return 2;
}


static void createstdfile (lua_State *L, FILE *f, const char *k,
                           const char *fname) {
  LStream *p = newprefile(L);
  p->f = f;
  p->closef = &io_noclose;
  if (k != NULL) {
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, k);  /* add file to registry */
  }
  lua_setfield(L, -2, fname);  /* add file to module */
}


LUAMOD_API int luaopen_io (lua_State *L) {
  FS_FUNCTION
  luaL_newlib(L, iolib);  /* new module */
  createmeta(L);
  /* create (and set) default files */
  createstdfile(L, stdin, IO_INPUT, "stdin");
  createstdfile(L, stdout, IO_OUTPUT, "stdout");
  createstdfile(L, stderr, NULL, "stderr");
  return 1;
}


static const luaL_Reg constexpr_iolib[] = {
  {"contents", contents},
  {NULL, NULL}
};

const Pluto::ConstexprLibrary Pluto::constexpr_io{ "io", constexpr_iolib };
