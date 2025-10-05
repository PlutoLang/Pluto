/*
** $Id: luaconf.h $
** Configuration file for Lua
** See Copyright Notice in lua.h
*/


#ifndef luaconf_h
#define luaconf_h

#include <limits.h>
#include <stddef.h>


/*
** ===================================================================
** General Configuration File for Lua
**
** Some definitions here can be changed externally, through the compiler
** (e.g., with '-D' options): They are commented out or protected
** by '#if !defined' guards. However, several other definitions
** should be changed directly here, either because they affect the
** Lua ABI (by making the changes here, you ensure that all software
** connected to Lua, such as C libraries, will be compiled with the same
** configuration); or because they are seldom changed.
**
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
** {====================================================================
** System Configuration: macros to adapt (if needed) Lua to some
** particular platform, for instance restricting it to C89.
** =====================================================================
*/

/*
@@ LUA_USE_C89 controls the use of non-ISO-C89 features.
** Define it if you want Lua to avoid the use of a few C99 features
** or Windows-specific features on Windows.
*/
/* #define LUA_USE_C89 */

/* [Pluto] Platform auto-detection */
#if !defined(LUA_USE_LINUX) && defined(__linux__)
  #define LUA_USE_LINUX
#endif
#if !defined(LUA_USE_MACOSX) && defined(__APPLE__) && defined(__MACH__)
  #define LUA_USE_MACOSX
#endif
#if !defined(LUA_USE_IOS) && defined(__APPLE__) && !defined(__MACH__)
  #define LUA_USE_IOS
#endif

/*
** By default, Lua on Windows use (some) specific Windows features
*/
#if !defined(LUA_USE_C89) && defined(_WIN32) && !defined(_WIN32_WCE)
#define LUA_USE_WINDOWS  /* enable goodies for regular Windows */
#endif


#if defined(LUA_USE_WINDOWS)
#define LUA_DL_DLL	/* enable support for DLL */
#endif


#if defined(LUA_USE_LINUX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* needs an extra library: -ldl */
#endif


#if defined(LUA_USE_MACOSX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* MacOS does not need -ldl */
#endif


#if defined(LUA_USE_IOS)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN
#endif


/*
@@ LUAI_IS32INT is true iff 'int' has (at least) 32 bits.
*/
#define LUAI_IS32INT	((UINT_MAX >> 30) >= 3)

/* }================================================================== */



/*
** {==================================================================
** Configuration for Number types. These options should not be
** set externally, because any other code connected to Lua must
** use the same configuration.
** ===================================================================
*/

/*
@@ LUA_INT_TYPE defines the type for Lua integers.
@@ LUA_FLOAT_TYPE defines the type for Lua floats.
** Lua should work fine with any mix of these options supported
** by your C compiler. The usual configurations are 64-bit integers
** and 'double' (the default), 32-bit integers and 'float' (for
** restricted platforms), and 'long'/'double' (for C compilers not
** compliant with C99, which may not have support for 'long long').
*/

/* predefined options for LUA_INT_TYPE */
#define LUA_INT_INT		1
#define LUA_INT_LONG		2
#define LUA_INT_LONGLONG	3

/* predefined options for LUA_FLOAT_TYPE */
#define LUA_FLOAT_FLOAT		1
#define LUA_FLOAT_DOUBLE	2
#define LUA_FLOAT_LONGDOUBLE	3


/* Default configuration ('long long' and 'double', for 64-bit Lua) */
#define LUA_INT_DEFAULT		LUA_INT_LONGLONG
#define LUA_FLOAT_DEFAULT	LUA_FLOAT_DOUBLE


/*
@@ LUA_32BITS enables Lua with 32-bit integers and 32-bit floats.
*/
#define LUA_32BITS	0


/*
@@ LUA_C89_NUMBERS ensures that Lua uses the largest types available for
** C89 ('long' and 'double'); Windows always has '__int64', so it does
** not need to use this case.
*/
#if defined(LUA_USE_C89) && !defined(LUA_USE_WINDOWS)
#define LUA_C89_NUMBERS		1
#else
#define LUA_C89_NUMBERS		0
#endif


#if LUA_32BITS		/* { */
/*
** 32-bit integers and 'float'
*/
#if LUAI_IS32INT  /* use 'int' if big enough */
#define LUA_INT_TYPE	LUA_INT_INT
#else  /* otherwise use 'long' */
#define LUA_INT_TYPE	LUA_INT_LONG
#endif
#define LUA_FLOAT_TYPE	LUA_FLOAT_FLOAT

#elif LUA_C89_NUMBERS	/* }{ */
/*
** largest types available for C89 ('long' and 'double')
*/
#define LUA_INT_TYPE	LUA_INT_LONG
#define LUA_FLOAT_TYPE	LUA_FLOAT_DOUBLE

#else		/* }{ */
/* use defaults */

#define LUA_INT_TYPE	LUA_INT_DEFAULT
#define LUA_FLOAT_TYPE	LUA_FLOAT_DEFAULT

#endif				/* } */


/* }================================================================== */



/*
** {==================================================================
** Configuration for Paths.
** ===================================================================
*/

/*
** LUA_PATH_SEP is the character that separates templates in a path.
** LUA_PATH_MARK is the string that marks the substitution points in a
** template.
** LUA_EXEC_DIR in a Windows path is replaced by the executable's
** directory.
*/
#define LUA_PATH_SEP            ";"
#define LUA_PATH_MARK           "?"
#define LUA_EXEC_DIR            "!"


/*
@@ LUA_PATH_DEFAULT is the default path that Lua uses to look for
** Lua libraries.
@@ LUA_CPATH_DEFAULT is the default path that Lua uses to look for
** C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/

#define LUA_VDIR	LUA_VERSION_MAJOR "." LUA_VERSION_MINOR
#if defined(_WIN32)	/* { */
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define LUA_LDIR	"!\\lua\\"
#define LUA_CDIR	"!\\"
#define LUA_SHRDIR	"!\\..\\share\\lua\\" LUA_VDIR "\\"

#if !defined(LUA_PATH_DEFAULT)
#define LUA_PATH_DEFAULT  \
        LUA_LDIR"?.lua;"  LUA_LDIR"?\\init.lua;" \
        LUA_CDIR"?.lua;"  LUA_CDIR"?\\init.lua;" \
        LUA_SHRDIR"?.lua;" LUA_SHRDIR"?\\init.lua;" \
        ".\\?.lua;" ".\\?\\init.lua;" \
        ".\\?.pluto;" ".\\?\\init.pluto"
#endif

#if !defined(LUA_CPATH_DEFAULT)
#define LUA_CPATH_DEFAULT \
        LUA_CDIR"?.dll;" \
        LUA_CDIR"..\\lib\\lua\\" LUA_VDIR "\\?.dll;" \
        LUA_CDIR"loadall.dll;" ".\\?.dll"
#endif

#else			/* }{ */

#define LUA_ROOT	"/usr/local/"
#define LUA_LDIR	LUA_ROOT "share/lua/" LUA_VDIR "/"
#define LUA_CDIR	LUA_ROOT "lib/lua/" LUA_VDIR "/"

#if !defined(LUA_PATH_DEFAULT)
#define LUA_PATH_DEFAULT  \
        LUA_LDIR"?.lua;"  LUA_LDIR"?/init.lua;" \
        LUA_CDIR"?.lua;"  LUA_CDIR"?/init.lua;" \
        "./?.lua;" "./?/init.lua;" \
        "./?.pluto;" "./?/init.pluto"
#endif

#if !defined(LUA_CPATH_DEFAULT)
#define LUA_CPATH_DEFAULT \
        LUA_CDIR"?.so;" LUA_CDIR"loadall.so;" "./?.so"
#endif

#endif			/* } */


/*
@@ LUA_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows Lua automatically uses "\".)
*/
#if !defined(LUA_DIRSEP)

#if defined(_WIN32)
#define LUA_DIRSEP	"\\"
#else
#define LUA_DIRSEP	"/"
#endif

#endif

/*
** LUA_IGMARK is a mark to ignore all after it when building the
** module name (e.g., used to build the luaopen_ function name).
** Typically, the suffix after the mark is the module version,
** as in "mod-v1.2.so".
*/
#define LUA_IGMARK		"-"

/* }================================================================== */


/*
** {==================================================================
** Marks for exported symbols in the C code
** ===================================================================
*/

/*
@@ LUA_API is a mark for all core API functions.
@@ LUALIB_API is a mark for all auxiliary library functions.
@@ LUAMOD_API is a mark for all standard library opening functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** LUA_BUILD_AS_DLL to get it).
*/
#if defined(LUA_BUILD_AS_DLL)
  #if defined(LUA_CORE) || defined(LUA_LIB)
    #define PLUTO_DLLSPEC __declspec(dllexport)
  #else
    #define PLUTO_DLLSPEC __declspec(dllimport)
  #endif
#else
  #ifdef __EMSCRIPTEN__
    #include "emscripten.h"
    #define PLUTO_DLLSPEC EMSCRIPTEN_KEEPALIVE
  #elif !defined(_WIN32)
    #define PLUTO_DLLSPEC __attribute__((visibility("default")))
  #else
    #define PLUTO_DLLSPEC
  #endif
#endif

// Additions by Pluto that are not compatible with `extern "C"` use PLUTO_API instead of LUA_API.
#define PLUTO_API	PLUTO_DLLSPEC

#ifdef __cplusplus
  #define LUA_API          extern "C" PLUTO_API
  #define LUA_API_NORETURN extern "C" [[noreturn]] PLUTO_API
#else
  #define LUA_API          PLUTO_API
  #define LUA_API_NORETURN PLUTO_API
#endif

/*
** More often than not the libs go together with the core.
*/
#define LUALIB_API	LUA_API
#define LUAMOD_API	LUA_API

#define LUALIB_API_NORETURN	LUA_API_NORETURN

#define PLUTOLIB_API	PLUTO_API


/*
@@ LUAI_FUNC is a mark for all extern functions that are not to be
** exported to outside modules.
@@ LUAI_DDEF and LUAI_DDEC are marks for all extern (const) variables,
** none of which to be exported to outside modules (LUAI_DDEF for
** definitions and LUAI_DDEC for declarations).
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when Lua is compiled as a shared library. Not all elf targets support
** this attribute. Unfortunately, gcc does not offer a way to check
** whether the target offers that support, and those without support
** give a warning about it. To avoid these warnings, change to the
** default definition.
*/
#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
    defined(__ELF__)		/* { */
#define LUAI_FUNC	__attribute__((visibility("internal"))) extern
#else				/* }{ */
#define LUAI_FUNC	extern
#endif				/* } */

#define LUAI_DDEC(dec)	LUAI_FUNC dec
#define LUAI_DDEF	/* empty */

/* }================================================================== */


/*
** {==================================================================
** Compatibility with previous versions
** ===================================================================
*/

/*
@@ LUA_COMPAT_5_3 controls other macros for compatibility with Lua 5.3.
** You can define it to get all options, or change specific options
** to fit your specific needs.
*/
#if defined(LUA_COMPAT_5_3)	/* { */

/*
@@ LUA_COMPAT_MATHLIB controls the presence of several deprecated
** functions in the mathematical library.
** (These functions were already officially removed in 5.3;
** nevertheless they are still available here.)
*/
#define LUA_COMPAT_MATHLIB

/*
@@ LUA_COMPAT_APIINTCASTS controls the presence of macros for
** manipulating other integer types (lua_pushunsigned, lua_tounsigned,
** luaL_checkint, luaL_checklong, etc.)
** (These macros were also officially removed in 5.3, but they are still
** available here.)
*/
#define LUA_COMPAT_APIINTCASTS


/*
@@ LUA_COMPAT_LT_LE controls the emulation of the '__le' metamethod
** using '__lt'.
*/
#define LUA_COMPAT_LT_LE


/*
@@ The following macros supply trivial compatibility for some
** changes in the API. The macros themselves document how to
** change your code to avoid using them.
** (Once more, these macros were officially removed in 5.3, but they are
** still available here.)
*/
#define lua_strlen(L,i)		lua_rawlen(L, (i))

#define lua_objlen(L,i)		lua_rawlen(L, (i))

#define lua_equal(L,idx1,idx2)		lua_compare(L,(idx1),(idx2),LUA_OPEQ)
#define lua_lessthan(L,idx1,idx2)	lua_compare(L,(idx1),(idx2),LUA_OPLT)

#endif				/* } */

/* }================================================================== */



/*
** {==================================================================
** Configuration for Numbers (low-level part).
** Change these definitions if no predefined LUA_FLOAT_* / LUA_INT_*
** satisfy your needs.
** ===================================================================
*/

/*
@@ LUAI_UACNUMBER is the result of a 'default argument promotion'
@@ over a floating number.
@@ l_floatatt(x) corrects float attribute 'x' to the proper float type
** by prefixing it with one of FLT/DBL/LDBL.
@@ LUA_NUMBER_FRMLEN is the length modifier for writing floats.
@@ LUA_NUMBER_FMT is the format for writing floats.
@@ lua_number2str converts a float to a string.
@@ l_mathop allows the addition of an 'l' or 'f' to all math operations.
@@ l_floor takes the floor of a float.
@@ lua_str2number converts a decimal numeral to a number.
*/


/* The following definitions are good for most cases here */

#define l_floor(x)		(l_mathop(floor)(x))

#define lua_number2str(s,sz,n)  \
    l_sprintf((s), sz, LUA_NUMBER_FMT, (LUAI_UACNUMBER)(n))

/*
@@ lua_numbertointeger converts a float number with an integral value
** to an integer, or returns 0 if float is not within the range of
** a lua_Integer.  (The range comparisons are tricky because of
** rounding. The tests here assume a two-complement representation,
** where MININTEGER always has an exact representation as a float;
** MAXINTEGER may not have one, and therefore its conversion to float
** may have an ill-defined value.)
*/
#define lua_numbertointeger(n,p) \
  ((n) >= (LUA_NUMBER)(LUA_MININTEGER) && \
   (n) < -(LUA_NUMBER)(LUA_MININTEGER) && \
      (*(p) = (LUA_INTEGER)(n), 1))


/* now the variable definitions */

#if LUA_FLOAT_TYPE == LUA_FLOAT_FLOAT		/* { single float */

#define LUA_NUMBER	float

#define l_floatatt(n)		(FLT_##n)

#define LUAI_UACNUMBER	double

#define LUA_NUMBER_FRMLEN	""
#define LUA_NUMBER_FMT		"%.7g"

#define l_mathop(op)		op##f

#define lua_str2number(s,p)	strtof((s), (p))


#elif LUA_FLOAT_TYPE == LUA_FLOAT_LONGDOUBLE	/* }{ long double */

#define LUA_NUMBER	long double

#define l_floatatt(n)		(LDBL_##n)

#define LUAI_UACNUMBER	long double

#define LUA_NUMBER_FRMLEN	"L"
#define LUA_NUMBER_FMT		"%.19Lg"

#define l_mathop(op)		op##l

#define lua_str2number(s,p)	strtold((s), (p))

#elif LUA_FLOAT_TYPE == LUA_FLOAT_DOUBLE	/* }{ double */

#define LUA_NUMBER	double

#define l_floatatt(n)		(DBL_##n)

#define LUAI_UACNUMBER	double

#define LUA_NUMBER_FRMLEN	""
#define LUA_NUMBER_FMT		"%.14g"

#define l_mathop(op)		op

#define lua_str2number(s,p)	strtod((s), (p))

#else						/* }{ */

#error "numeric float type not defined"

#endif					/* } */



/*
@@ LUA_UNSIGNED is the unsigned version of LUA_INTEGER.
@@ LUAI_UACINT is the result of a 'default argument promotion'
@@ over a LUA_INTEGER.
@@ LUA_INTEGER_FRMLEN is the length modifier for reading/writing integers.
@@ LUA_INTEGER_FMT is the format for writing integers.
@@ LUA_MAXINTEGER is the maximum value for a LUA_INTEGER.
@@ LUA_MININTEGER is the minimum value for a LUA_INTEGER.
@@ LUA_MAXUNSIGNED is the maximum value for a LUA_UNSIGNED.
@@ lua_integer2str converts an integer to a string.
*/


/* The following definitions are good for most cases here */

#define LUA_INTEGER_FMT		"%" LUA_INTEGER_FRMLEN "d"

#define LUAI_UACINT		LUA_INTEGER

#define lua_integer2str(s,sz,n)  \
    l_sprintf((s), sz, LUA_INTEGER_FMT, (LUAI_UACINT)(n))

/*
** use LUAI_UACINT here to avoid problems with promotions (which
** can turn a comparison between unsigneds into a signed comparison)
*/
#define LUA_UNSIGNED		unsigned LUAI_UACINT


/* now the variable definitions */

#if LUA_INT_TYPE == LUA_INT_INT		/* { int */

#define LUA_INTEGER		int
#define LUA_INTEGER_FRMLEN	""

#define LUA_MAXINTEGER		INT_MAX
#define LUA_MININTEGER		INT_MIN

#define LUA_MAXUNSIGNED		UINT_MAX

#elif LUA_INT_TYPE == LUA_INT_LONG	/* }{ long */

#define LUA_INTEGER		long
#define LUA_INTEGER_FRMLEN	"l"

#define LUA_MAXINTEGER		LONG_MAX
#define LUA_MININTEGER		LONG_MIN

#define LUA_MAXUNSIGNED		ULONG_MAX

#elif LUA_INT_TYPE == LUA_INT_LONGLONG	/* }{ long long */

/* use presence of macro LLONG_MAX as proxy for C99 compliance */
#if defined(LLONG_MAX)		/* { */
/* use ISO C99 stuff */

#define LUA_INTEGER		long long
#define LUA_INTEGER_FRMLEN	"ll"

#define LUA_MAXINTEGER		LLONG_MAX
#define LUA_MININTEGER		LLONG_MIN

#define LUA_MAXUNSIGNED		ULLONG_MAX

#elif defined(LUA_USE_WINDOWS) /* }{ */
/* in Windows, can use specific Windows types */

#define LUA_INTEGER		__int64
#define LUA_INTEGER_FRMLEN	"I64"

#define LUA_MAXINTEGER		_I64_MAX
#define LUA_MININTEGER		_I64_MIN

#define LUA_MAXUNSIGNED		_UI64_MAX

#else				/* }{ */

#error "Compiler does not support 'long long'. Use option '-DLUA_32BITS' \
  or '-DLUA_C89_NUMBERS' (see file 'luaconf.h' for details)"

#endif				/* } */

#else				/* }{ */

#error "numeric integer type not defined"

#endif				/* } */

/* }================================================================== */


/*
** {==================================================================
** Dependencies with C99 and other C details
** ===================================================================
*/

/*
@@ l_sprintf is equivalent to 'snprintf' or 'sprintf' in C89.
** (All uses in Lua have only one format item.)
*/
#if !defined(LUA_USE_C89)
#define l_sprintf(s,sz,f,i)	snprintf(s,sz,f,i)
#else
#define l_sprintf(s,sz,f,i)	((void)(sz), sprintf(s,f,i))
#endif


/*
@@ lua_strx2number converts a hexadecimal numeral to a number.
** In C99, 'strtod' does that conversion. Otherwise, you can
** leave 'lua_strx2number' undefined and Lua will provide its own
** implementation.
*/
#if !defined(LUA_USE_C89)
#define lua_strx2number(s,p)		lua_str2number(s,p)
#endif


/*
@@ lua_pointer2str converts a pointer to a readable string in a
** non-specified way.
*/
#define lua_pointer2str(buff,sz,p)	l_sprintf(buff,sz,"%p",p)


/*
@@ lua_number2strx converts a float to a hexadecimal numeral.
** In C99, 'sprintf' (with format specifiers '%a'/'%A') does that.
** Otherwise, you can leave 'lua_number2strx' undefined and Lua will
** provide its own implementation.
*/
#if !defined(LUA_USE_C89)
#define lua_number2strx(L,b,sz,f,n)  \
    ((void)L, l_sprintf(b,sz,f,(LUAI_UACNUMBER)(n)))
#endif


/*
** 'strtof' and 'opf' variants for math functions are not valid in
** C89. Otherwise, the macro 'HUGE_VALF' is a good proxy for testing the
** availability of these variants. ('math.h' is already included in
** all files that use these macros.)
*/
#if defined(LUA_USE_C89) || (defined(HUGE_VAL) && !defined(HUGE_VALF))
#undef l_mathop  /* variants not available */
#undef lua_str2number
#define l_mathop(op)		(lua_Number)op  /* no variant */
#define lua_str2number(s,p)	((lua_Number)strtod((s), (p)))
#endif


/*
@@ LUA_KCONTEXT is the type of the context ('ctx') for continuation
** functions.  It must be a numerical type; Lua will use 'intptr_t' if
** available, otherwise it will use 'ptrdiff_t' (the nearest thing to
** 'intptr_t' in C89)
*/
#define LUA_KCONTEXT	ptrdiff_t

#if !defined(LUA_USE_C89) && defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 199901L
#include <stdint.h>
#if defined(INTPTR_MAX)  /* even in C99 this type is optional */
#undef LUA_KCONTEXT
#define LUA_KCONTEXT	intptr_t
#endif
#endif


/*
@@ lua_getlocaledecpoint gets the locale "radix character" (decimal point).
** Change that if you do not want to use C locales. (Code using this
** macro must include the header 'locale.h'.)
*/
#if !defined(lua_getlocaledecpoint)
#define lua_getlocaledecpoint()		(localeconv()->decimal_point[0])
#endif


/*
** macros to improve jump prediction, used mostly for error handling
** and debug facilities. (Some macros in the Lua API use these macros.
** Define LUA_NOBUILTIN if you do not want '__builtin_expect' in your
** code.)
*/
#if !defined(luai_likely)

#if (defined(__GNUC__) || defined(__CLANG__)) && !defined(LUA_NOBUILTIN)
#define luai_likely(x)		(__builtin_expect(((x) != 0), 1))
#define luai_unlikely(x)	(__builtin_expect(((x) != 0), 0))
#else
#define luai_likely(x)		(x)
#define luai_unlikely(x)	(x)
#endif

#endif


#if defined(LUA_CORE) || defined(LUA_LIB)
/* shorter names for Lua's own use */
#define l_likely(x)	luai_likely(x)
#define l_unlikely(x)	luai_unlikely(x)
#endif



/* }================================================================== */


/*
** {==================================================================
** Language Variations
** =====================================================================
*/

/*
@@ LUA_NOCVTN2S/LUA_NOCVTS2N control how Lua performs some
** coercions. Define LUA_NOCVTN2S to turn off automatic coercion from
** numbers to strings. Define LUA_NOCVTS2N to turn off automatic
** coercion from strings to numbers.
*/
/* #define LUA_NOCVTN2S */
/* #define LUA_NOCVTS2N */


/*
@@ LUA_USE_APICHECK turns on several consistency checks on the C API.
** Define it as a help when debugging C code.
*/
#if defined(LUA_USE_APICHECK)
#include <assert.h>
#define luai_apicheck(l,e)	assert(e)
#endif

/* }================================================================== */


/*
** {==================================================================
** Macros that affect the API and must be stable (that is, must be the
** same when you compile Lua and when you compile code that links to
** Lua).
** =====================================================================
*/

/*
@@ LUAI_MAXSTACK limits the size of the Lua stack.
** CHANGE it if you need a different limit. This limit is arbitrary;
** its only purpose is to stop Lua from consuming unlimited stack
** space (and to reserve some numbers for pseudo-indices).
** (It must fit into max(size_t)/32.)
*/
#if LUAI_IS32INT
#define LUAI_MAXSTACK		1000000
#else
#define LUAI_MAXSTACK		15000
#endif


/*
@@ LUA_EXTRASPACE defines the size of a raw memory area associated with
** a Lua state with very fast access.
** CHANGE it if you need a different size.
*/
#define LUA_EXTRASPACE		(sizeof(void *))


/*
@@ LUA_IDSIZE gives the maximum size for the description of the source
** of a function in debug information.
** CHANGE it if you want a different size.
*/
#define LUA_IDSIZE	60


/*
@@ LUAL_BUFFERSIZE is the initial buffer size used by the lauxlib
** buffer system.
*/
#define LUAL_BUFFERSIZE   ((int)(16 * sizeof(void*) * sizeof(lua_Number)))


/*
@@ LUAI_MAXALIGN defines fields that, when used in a union, ensure
** maximum alignment for the other items in that union.
*/
#define LUAI_MAXALIGN  lua_Number n; double u; void *s; lua_Integer i; long l

/* }================================================================== */

/*
** {====================================================================
** Pluto Configuration
** =====================================================================}
*/

// If defined, Pluto errors will use ANSI color codes.
//#define PLUTO_USE_COLORED_OUTPUT

// If defined, Pluto will exclude code snippets from error messages to make them shorter.
//#define PLUTO_SHORT_ERRORS

// If defined, Pluto won't assume that source files are UTF-8 encoded and restrict valid symbol names.
//#define PLUTO_NO_UTF8

// If defined, Pluto will use a jumptable in the VM even if not compiled via GCC or Clang.
// This will generally improve runtime performance but can add minutes to compile time, depending on the setup.
//#define PLUTO_FORCE_JUMPTABLE

// If defined, Pluto won't imbue tables with a metatable by default.
//#define PLUTO_NO_DEFAULT_TABLE_METATABLE

// If defined, Pluto will add table.isfrozen & table.freeze to the standard library,
// lua_freezetable, lua_istablefrozen, & lua_erriffrozen to the C API,
// and all the hooks required to make it work. Note that coverage may not be perfect.
//#define PLUTO_ENABLE_TABLE_FREEZING

// If defined, Pluto will cache the bytecode of text files that parsed without warnings or errors,
// and if the contents remained unchanged, it'll load the bytecode instead of reparsing the file.
// The worst-case scenario for this optimization is small files and files that change often,
// but even then, the overhead should be at most 1ms on modern systems.
//#define PLUTO_PARSER_CACHE

/*
** {====================================================================
** Pluto Configuration: Warnings
** =====================================================================}
*/

// If defined, the "global-shadow" warning is enabled by default.
//#define PLUTO_WARN_GLOBAL_SHADOW

// The list of globals covered by the "global-shadow" warning.
#ifndef PLUTO_COMMON_GLOBAL_NAMES
#define PLUTO_COMMON_GLOBAL_NAMES "table","string","arg"
#endif

// If defined, the "non-portable-code" warning is enabled by default.
//#define PLUTO_WARN_NON_PORTABLE_CODE

// If defined, the "non-portable-bytecode" warning is enabled by default.
//#define PLUTO_WARN_NON_PORTABLE_BYTECODE

// If defined, the "non-portable-name" warning is enabled by default.
//#define PLUTO_WARN_NON_PORTABLE_NAME

// If defined, the "var-shadow" warning is disabled by default.
//#define PLUTO_NO_WARN_VAR_SHADOW

// If defined, the "type-mismatch" warning is disabled by default.
//#define PLUTO_NO_WARN_TYPE_MISMATCH

// If defined, the "unreachable-code" warning is disabled by default.
//#define PLUTO_NO_WARN_UNREACHABLE_CODE

// If defined, the "excessive-arguments" warning is disabled by default.
//#define PLUTO_NO_WARN_EXCESSIVE_ARGUMENTS

// If defined, the "deprecated"," warning is disabled by default.
//#define PLUTO_NO_WARN_DEPRECATED

// If defined, the "bad-practice" warning is disabled by default.
//#define PLUTO_NO_WARN_BAD_PRACTICE

// If defined, the "possible-typo" warning is disabled by default.
//#define PLUTO_NO_WARN_POSSIBLE_TYPO

// If defined, the "unannotated-fallthrough" warning is disabled by default.
//#define PLUTO_NO_WARN_UNANNOTATED_FALLTHROUGH

// If defined, the "discarded-return" warning is disabled by default.
//#define PLUTO_NO_WARN_DISCARDED_RETURN

// If defined, the "field-shadow" warning is disabled by default.
//#define PLUTO_NO_WARN_FIELD_SHADOW

// If defined, the "unused" warning is disabled by default.
//#define PLUTO_NO_WARN_UNUSED


/*
** {====================================================================
** Pluto Configuration: Compatibility
** =====================================================================}
*/

// If defined, Pluto will assign 'pluto_' to new keywords which break previously valid Lua identifiers.
// So, for example, the 'switch' keyword becomes 'pluto_switch'. The 'pluto_' variants are valid even if this is not defined.
// As of Pluto 0.7.0, scripts can individually set compatibility modes via 'pluto_use'.
//#define PLUTO_COMPATIBLE_MODE

#ifdef PLUTO_COMPATIBLE_MODE
    #define PLUTO_COMPATIBLE_SWITCH
    #define PLUTO_COMPATIBLE_CONTINUE
    #define PLUTO_COMPATIBLE_ENUM
    #define PLUTO_COMPATIBLE_NEW
    #define PLUTO_COMPATIBLE_CLASS
    #define PLUTO_COMPATIBLE_PARENT
    #define PLUTO_COMPATIBLE_EXPORT
    #define PLUTO_COMPATIBLE_TRY
    #define PLUTO_COMPATIBLE_CATCH
#endif

// If defined, Pluto's automatic keyword detection will more aggressively disable keywords if they're not used exactly as expected.
// This will help when scripters use these keywords as globals across files or before their definition.
//#define PLUTO_PARANOID_KEYWORD_DETECTION

// If defined, Pluto disables optimisations of Lua macros that would make your code unable to be linked
// against Lua if your code is using these macros with Pluto's definitions.
//#define PLUTO_LUA_LINKABLE

/*
** {====================================================================
** Pluto Configuration: Optional keywords
** =====================================================================}
*/

// If defined, Pluto will imply 'pluto_use global' at the beginning of every script.
//#define PLUTO_USE_GLOBAL

/*
** {====================================================================
** Pluto Configuration: Infinite Loop Prevention (ILP)
**
** This is only useful in game regions, where a long loop may block the main thread and crash the game.
** These places usually implement a yield (or wait) function, which can be detected and hooked to reset iterations.
** =====================================================================}
*/

// If defined, Pluto will attempt to prevent infinite loops.
//#define PLUTO_ILP_ENABLE

#ifdef PLUTO_ILP_ENABLE
/*
** This is the maximum amount of backward jumps permitted in a singular loop block.
** If exceeded, the backward jump is ignored to escape the loop.
*/
#ifndef PLUTO_ILP_MAX_ITERATIONS
#define PLUTO_ILP_MAX_ITERATIONS			1'000'000
#endif

// If you want (i.e) `luaB_next` to reset iteration counters, define as `luaB_next`.
// #define PLUTO_ILP_HOOK_FUNCTION		luaB_next

// If defined, Pluto won't throw an error and instead just break out of the loop.
//#define PLUTO_ILP_SILENT_BREAK

// Allows you to customise how an ILP violation is raised to the runtime (or not).
#ifdef PLUTO_ILP_SILENT_BREAK
  #define PLUTO_ILP_ERROR ;
#else
  #ifndef PLUTO_ILP_ERROR
    #define PLUTO_ILP_ERROR luaG_runerror(L, "infinite loop detected (exceeded max iterations: %d)", PLUTO_ILP_MAX_ITERATIONS);
  #endif
#endif

#endif // PLUTO_ILP_ENABLE

/*
** {====================================================================
** Pluto Configuration: Execution Time Limit (ETL)
**
** This is only useful in sandbox environments where stalling is absolutely unacceptable.
** =====================================================================}
*/

//#define PLUTO_ETL_ENABLE

#ifdef PLUTO_ETL_ENABLE
/*
** This is the maximum amount of nanoseconds the VM is allowed to run.
*/
#ifndef PLUTO_ETL_NANOS
#define PLUTO_ETL_NANOS			1'000'000 /* 1ms */
#endif

/*
** This can be used to execute custom code when the time limit is exceeded and
** the VM is about to be terminated.
*/
#ifndef PLUTO_ETL_TIMESUP
#define PLUTO_ETL_TIMESUP luaG_runerror(L, "Execution time limit exceeded");
#endif
#endif

/*
** {====================================================================
** Pluto Configuration: Memory Limit
**
** For sandbox environments. This changes the memory allocator in luaL_newstate.
** =====================================================================}
*/

//#define PLUTO_MEMORY_LIMIT 64'000'000 /* 64 MB (megabytes, not mebibytes!) */

/*
** {====================================================================
** Pluto Configuration: VM Dump
** =====================================================================}
*/

// If defined, Pluto will print every VM instruction that is ran.
// Note that you can modify lua_writestring to redirect output.
//#define PLUTO_VMDUMP

#ifdef PLUTO_VMDUMP
/* Example:
**  #define vmDumpIgnore \
**      OP_LOADI \
**      OP_LOADF
*/

// Opcodes listed in this structure are a blacklist. They not be printed when VM dumping.
#define vmDumpIgnore


// Opcodes listed in this structure are a whitelist. They are only printed when VM dumping.
#define vmDumpAllow

// If defined, Pluto will use vmDumpAllow instead of vmDumpIgnore.
//#define PLUTO_VMDUMP_WHITELIST

// Defines under what circumstances the VM Dump is active.
#ifndef PLUTO_VMDUMP_COND
#define PLUTO_VMDUMP_COND(L) true
#endif

#endif // PLUTO_VMDUMP

/*
** {====================================================================
** Pluto Configuration: Content Moderation
** =====================================================================}
*/

// If defined, Pluto will not load compiled Lua or Pluto code.
//#define PLUTO_DISABLE_COMPILED

// If defined, the provided function will be called as bool(lua_State* L, const char* code).
// If it returns false, a Lua error is raised.
//#define PLUTO_LOAD_HOOK ContmodOnLoad

// If defined, the provided function will be called as bool(lua_State* L, const char* filename).
// If it returns false, a Lua error is raised.
// This will affect require, dofile, and $include.
//#define PLUTO_LOADFILE_HOOK ContmodOnLoadfile

// It is possible to pass a reader function to the load function.
// Pluto currently offers no way to moderate code loaded like this,
// so you may define this to disable this method of code-loading.
//#define PLUTO_DISABLE_UNMODERATED_LOAD

// If defined, the provided function will be called as bool(lua_State* L, const char* path).
// If it returns false, a Lua eror is raised.
// This will affect require and package.loadlib.
//#define PLUTO_LOADCLIB_HOOK ContmodOnLoadCLib

// If defined, Pluto will not load the io library, exclude os.remove and os.rename, and error on any use of the $include directive.
// It's highly suggested in most cases to define PLUTO_NO_OS_EXECUTE below too, since os.execute can be used for filesystem access. 
// It's suggested you implement PLUTO_LOADCLIB_HOOK, etc, for even more powerful coverage. Package.loadlib can still load other Pluto/Lua libraries and use their lua_CFunction objects.
//#define PLUTO_NO_FILESYSTEM

// Disables os.execute & io.popen.
//#define PLUTO_NO_OS_EXECUTE

// Eliminate any loading of any binaries. This removes package.loadlib & ffi.open and prevents 'require' from loading any C modules or shared libraries.
//#define PLUTO_NO_BINARIES

#ifdef PLUTO_NO_BINARIES
#define PLUTO_NO_BINARIES_FAIL luaL_error(L, "binary modules cannot be loaded in this environment");
#endif

// If defined, all HTTP requests will fail.
// Note that the 'socket' library can still be used to the same effect (with more effort).
//#define PLUTO_DISABLE_HTTP_COMPLETELY

// If defined, the provided function will be called as bool(lua_State* L, const char* url).
// If it returns false, a Lua error is raised.
// Note that the 'socket' library can still be used to the same effect (with more effort).
//#define PLUTO_HTTP_REQUEST_HOOK ContmodOnHttpRequest

// If defined, the provided function will be called as bool(lua_State* L, const char* path)
// for any attempt to read a file's contents or metadata. The path will be UTF-8 encoded.
// If it returns false, a Lua error is raised.
//#define PLUTO_READ_FILE_HOOK ContmodOnReadFile

// If defined, the provided function will be called as bool(lua_State* L, const char* path)
// for any attempt to write a file's contents or metadata. The path will be UTF-8 encoded.
// If it returns false, a Lua error is raised.
//#define PLUTO_WRITE_FILE_HOOK ContmodOnWriteFile

// If defined, the provided function will be called as bool(lua_State* L, void* addr)
// for any attempt to call a foreign function.
// If it returns false, a Lua error is raised.
//#define PLUTO_FFI_CALL_HOOK ContmodOnFfiCall

/*
** {====================================================================
** Pluto color macros.
** =====================================================================}
*/

#ifdef PLUTO_USE_COLORED_OUTPUT // Don't need to write any 'ifdef' macro logic inside of Pluto::ErrorMessage.
#define ESC "\x1B"

#define BLK ESC "[0;30m"
#define RED ESC "[0;31m"
#define GRN ESC "[0;32m"
#define YEL ESC "[0;33m"
#define BLU ESC "[0;34m"
#define MAG ESC "[0;35m"
#define CYN ESC "[0;36m"
#define WHT ESC "[0;37m"

//Regular bold text
#define BBLK ESC "[1;30m"
#define BRED ESC "[1;31m"
#define BGRN ESC "[1;32m"
#define BYEL ESC "[1;33m"
#define BBLU ESC "[1;34m"
#define BMAG ESC "[1;35m"
#define BCYN ESC "[1;36m"
#define BWHT ESC "[1;37m"

//Regular underline text
#define UBLK ESC "[4;30m"
#define URED ESC "[4;31m"
#define UGRN ESC "[4;32m"
#define UYEL ESC "[4;33m"
#define UBLU ESC "[4;34m"
#define UMAG ESC "[4;35m"
#define UCYN ESC "[4;36m"
#define UWHT ESC "[4;37m"

//Regular background
#define BLKB ESC "[40m"
#define REDB ESC "[41m"
#define GRNB ESC "[42m"
#define YELB ESC "[43m"
#define BLUB ESC "[44m"
#define MAGB ESC "[45m"
#define CYNB ESC "[46m"
#define WHTB ESC "[47m"

//High intensty background 
#define BLKHB ESC "[0;100m"
#define REDHB ESC "[0;101m"
#define GRNHB ESC "[0;102m"
#define YELHB ESC "[0;103m"
#define BLUHB ESC "[0;104m"
#define MAGHB ESC "[0;105m"
#define CYNHB ESC "[0;106m"
#define WHTHB ESC "[0;107m"

//High intensty text
#define HBLK ESC "[0;90m"
#define HRED ESC "[0;91m"
#define HGRN ESC "[0;92m"
#define HYEL ESC "[0;93m"
#define HBLU ESC "[0;94m"
#define HMAG ESC "[0;95m"
#define HCYN ESC "[0;96m"
#define HWHT ESC "[0;97m"

//Bold high intensity text
#define BHBLK ESC "[1;90m"
#define BHRED ESC "[1;91m"
#define BHGRN ESC "[1;92m"
#define BHYEL ESC "[1;93m"
#define BHBLU ESC "[1;94m"
#define BHMAG ESC "[1;95m"
#define BHCYN ESC "[1;96m"
#define BHWHT ESC "[1;97m"

//Reset
#define RESET ESC "[0m"
#define CRESET ESC "[0m"
#define COLOR_RESET ESC "[0m"
#else // PLUTO_USE_COLORED_OUTPUT
#define ESC ""
#define BLK ESC
#define RED ESC
#define GRN ESC
#define YEL ESC
#define BLU ESC
#define MAG ESC
#define CYN ESC
#define WHT ESC
#define BBLK ESC
#define BRED ESC
#define BGRN ESC
#define BYEL ESC
#define BBLU ESC
#define BMAG ESC
#define BCYN ESC
#define BWHT ESC
#define UBLK ESC
#define URED ESC
#define UGRN ESC
#define UYEL ESC
#define UBLU ESC
#define UMAG ESC
#define UCYN ESC
#define UWHT ESC
#define BLKB ESC
#define REDB ESC
#define GRNB ESC
#define YELB ESC
#define BLUB ESC
#define MAGB ESC
#define CYNB ESC
#define WHTB ESC
#define BLKHB ESC
#define REDHB ESC
#define GRNHB ESC
#define YELHB ESC
#define BLUHB ESC
#define MAGHB ESC
#define CYNHB ESC
#define WHTHB ESC
#define HBLK ESC
#define HRED ESC
#define HGRN ESC
#define HYEL ESC
#define HBLU ESC
#define HMAG ESC
#define HCYN ESC
#define HWHT ESC
#define BHBLK ESC
#define BHRED ESC
#define BHGRN ESC
#define BHYEL ESC
#define BHBLU ESC
#define BHMAG ESC
#define BHCYN ESC
#define BHWHT ESC
#define RESET ESC
#define CRESET ESC
#define COLOR_RESET ESC
#endif // PLUTO_USE_COLORED_OUTPUT


/* }================================================================== */
#endif
