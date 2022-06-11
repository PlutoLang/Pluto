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


/*
** By default, Lua on Windows use (some) specific Windows features
*/
#if !defined(LUA_USE_C89) && defined(_WIN32) && !defined(_WIN32_WCE)
#define LUA_USE_WINDOWS  /* enable goodies for regular Windows */
#endif


#if defined(LUA_USE_WINDOWS)
#define LUA_DL_DLL	/* enable support for DLL */
#define LUA_USE_C89	/* broadly, Windows is C89 */
#endif


#if defined(LUA_USE_LINUX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* needs an extra library: -ldl */
#endif


#if defined(LUA_USE_MACOSX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* MacOS does not need -ldl */
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
		".\\?.lua;" ".\\?\\init.lua"
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
		"./?.lua;" "./?/init.lua"
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
#if defined(LUA_BUILD_AS_DLL)	/* { */

#if defined(LUA_CORE) || defined(LUA_LIB)	/* { */
#define LUA_API __declspec(dllexport)
#else						/* }{ */
#define LUA_API __declspec(dllimport)
#endif						/* } */

#else				/* }{ */

#define LUA_API		extern

#endif				/* } */


/*
** More often than not the libs go together with the core.
*/
#define LUALIB_API	LUA_API
#define LUAMOD_API	LUA_API


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

#if defined(__GNUC__) && !defined(LUA_NOBUILTIN)
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
@@ of a function in debug information.
** CHANGE it if you want a different size.
*/
#define LUA_IDSIZE	60


/*
@@ LUAL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
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
** Pluto configuration macros.
** If a macro is followed by a commented-out undef, you can toggle it.
** =====================================================================}
*/

#define PLUTO_USE_COLORED_OUTPUT
// #undef PLUTO_USE_COLORED_OUTPUT

#define PLUTO_PARSER_ENABLE_WARNIGNS
// #undef PLUTO_PARSER_ENABLE_WARNIGNS

#ifdef PLUTO_PARSER_ENABLE_WARNIGNS
#define PLUTO_PARSER_WARNING_LOCALDEF
// #undef PLUTO_PARSER_WARNING_LOCALDEF
#endif

#ifdef PLUTO_USE_COLORED_OUTPUT
#define ERROR_DEFAULT_HERE					"\e[0;31m^ here\e[0;37m"
#else
#define ERROR_DEFAULT_HERE					"^ here"
#endif

// Missing identifier for local message.
#ifdef PLUTO_USE_COLORED_OUTPUT
#define ERROR_MISSING_LOCAL_NAME        	"\n\e[0;33m%s\n\e[0;32mnote: you may've forgot to name your local during declaration.\e[0;37m"
#else
#define ERROR_MISSING_LOCAL_NAME        	"%s\nnote: you may've forgot to name your local during declaration."
#endif

// Function object without initial identifier.
#ifdef PLUTO_USE_COLORED_OUTPUT
#define ERROR_UNFINISHED_FUNCTION       	"\n\e[0;33m%s\n\e[0;32mnote: You may've forgot to name your function during declaration.\n      Functions must be associated with names when they're declared.\n      Here's an example inside the PIL: https://www.lua.org/pil/5.html\e[0;37m"
#define ERROR_UNFINISHED_FUNCTION_SYN		"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected <name> to perform as an identifier.\e[0;37m"
#define ERROR_UNFINISHED_FUNCTION_HERE		"\e[0;31m        ^ here: missing function name.\e[0;37m"
#else
#define ERROR_UNFINISHED_FUNCTION       	"%s\nnote: You may've forgot to name your function during declaration.\n      Functions must be associated with names when they're declared.\n      Here's an example inside the PIL: https://www.lua.org/pil/5.html"
#define ERROR_UNFINISHED_FUNCTION_SYN		"syntax error: expected <name> to perform as an identifier."
#define ERROR_UNFINISHED_FUNCTION_HERE		"        ^ here: missing function name."
#endif

// Unfinished if statement, missing TK_THEN token.
#ifdef PLUTO_USE_COLORED_OUTPUT
#define ERROR_UNFINISHED_IF_STATEMENT   	"\n\e[0;33m%s\n\e[0;32mnote: You forgot to finish your condition with 'then'.\n      Pluto requires this symbol to append each condition.\n      If needed, here's an example of how to use the 'if' statement: https://www.lua.org/pil/4.3.1.html\e[0;37m"
#define ERROR_UNFINISHED_IF_STATEMENT_SYN 	"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'then' to delimit 'if' condition.\e[0;37m"
#define ERROR_UNFINISHED_IF_STATEMENT_HERE  "\e[0;31m      ^ here: missing 'then' symbol.\e[0;37m";
#else
#define ERROR_UNFINISHED_IF_STATEMENT   	"%s\nnote: You forgot to finish your condition with 'then'.\n      Pluto requires this symbol to append each condition.\n      If needed, here's an example of how to use the 'if' statement: https://www.lua.org/pil/4.3.1.html"
#define ERROR_UNFINISHED_IF_STATEMENT_SYN 	"syntax error: expected 'then' to delimit 'if' condition."
#define ERROR_UNFINISHED_IF_STATEMENT_HERE  "      ^ here: missing 'then' symbol.";
#endif

// Control structure (i.e, do) doesn't end with 'end'.
#ifdef PLUTO_USE_COLORED_OUTPUT
#define ERROR_UNFINISHED_STRUCTURE_IF 		"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate 'if' structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_DO		"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate 'do' structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_FOR		"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate 'for' structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_WHILE	"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate 'while' structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_FUNCTION	"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate 'function' structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_DEFAULT	"\e[0;31msyntax error\e[0;37m: \e[1;37mexpected 'end' to terminate control structure.\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_END		"\e[0;33m%s\n\e[0;32mnote: You forgot to close your control structure with 'end'.\n      Pluto requires this symbol to append each control structure.\n      If needed, here's an example of how to delimit control structures: https://www.lua.org/pil/4.3.html\e[0;37m"
#define ERROR_UNFINISHED_STRUCTURE_END_HERE "\e[0;31m^ after this\e[0;37m"
#else
#define ERROR_UNFINISHED_STRUCTURE_IF 		"syntax error: expected 'end' to terminate 'if' structure."
#define ERROR_UNFINISHED_STRUCTURE_DO		"syntax error: expected 'end' to terminate 'do' structure."
#define ERROR_UNFINISHED_STRUCTURE_FOR		"syntax error: expected 'end' to terminate 'for' structure."
#define ERROR_UNFINISHED_STRUCTURE_WHILE	"syntax error: expected 'end' to terminate 'while' structure."
#define ERROR_UNFINISHED_STRUCTURE_FUNCTION	"syntax error: expected 'end' to terminate 'function' structure."
#define ERROR_UNFINISHED_STRUCTURE_DEFAULT	"syntax error: expected 'end' to terminate control structure."
#define ERROR_UNFINISHED_STRUCTURE_END		"%s\nnote: You forgot to close your control structure with 'end'.\n      Pluto requires this symbol to append each control structure.\n      If needed, here's an example of how to delimit control structures: https://www.lua.org/pil/4.3.html"
#define ERROR_UNFINISHED_STRUCTURE_END_HERE "^ after this"
#endif

// Control structure (i.e, do) doesn't end with 'end'.
#define ERROR_UNFINISHED_STRUCTURE_UNT		"%s\nnote: You forgot to close your 'repeat' structure with 'until'.\n      This conditional delimits the loop's behavior & terminates the structure.\n      If needed, here's an example of how to delimit control structures: https://www.lua.org/pil/4.3.html"

/* }================================================================== */
#endif
