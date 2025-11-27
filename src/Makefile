# Makefile for building Lua
# See ../doc/readme.html for installation and customization instructions.

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT=

CXX= g++ -std=c++17 -O3 -flto -fvisibility=hidden -fPIC
CXXFLAGS= -Wno-multichar $(SYSCFLAGS) $(MYCFLAGS)
LDFLAGS= $(SYSLDFLAGS) $(MYLDFLAGS)
LIBS= -lm -Lvendor/Soup/soup -lsoup $(SYSLIBS) $(MYLIBS)

# Specifying DEBUG=0 (or any value) will remove any optimizations.
ifdef DEBUG
override CXX= g++ -std=c++17
endif

AR= ar rcu
RANLIB= ranlib
RM= rm -f
UNAME= uname

SYSCFLAGS=
SYSLDFLAGS=
SYSLIBS=

MYCFLAGS=
MYLDFLAGS=
MYLIBS=
MYOBJS=

# Special flags for compiler modules; -Os reduces code size.
CMCFLAGS=

# == END OF USER SETTINGS -- NO NEED TO CHANGE ANYTHING BELOW THIS LINE =======

PLATS= guess aix bsd freebsd generic linux linux-readline termux termux-readline macosx mingw mingwdll posix solaris

LUA_A=	libplutostatic.a
LUA_SO= libpluto.so
CORE_O=	lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o lundump.o lvm.o lzio.o
LIB_O=	lauxlib.o lbaselib.o lcorolib.o ldblib.o liolib.o lmathlib.o loadlib.o loslib.o lstrlib.o lcryptolib.o ltablib.o lutf8lib.o lassertlib.o lvector3lib.o lbase32.o lbase64.o ljson.o lurllib.o linit.o lstarlib.o lcatlib.o lhttplib.o lschedulerlib.o lsocketlib.o lbigint.o lxml.o lregex.o lffi.o lcanvas.o lbufferlib.o
BASE_O= $(CORE_O) $(LIB_O) $(MYOBJS)

LUA_T=	pluto
LUA_O=	lua.o

LUAC_T=	plutoc
LUAC_O=	luac.o

ALL_O= $(BASE_O) $(LUA_O) $(LUAC_O)
ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T) $(LUA_SO)
ALL_A= $(LUA_A)

# Targets start here.
default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(LUA_A): $(BASE_O)
	cd vendor/Soup/soup && $(MAKE) && cd ../..
	$(AR) $@ $(BASE_O) vendor/Soup/soup/*.o
	$(RANLIB) $@

$(LUA_SO): $(BASE_O) $(LUA_A)
	$(CXX) -shared -Wl,-soname,$(LUA_SO) -o $@ $? ${LIBS}

$(LUA_T): $(LUA_O) $(LUA_A)
	$(CXX) -o $@ $(LDFLAGS) -Wl,--export-dynamic $(LUA_O) $(LUA_A) $(LIBS)

$(LUAC_T): $(LUAC_O) $(LUA_A)
	$(CXX) -o $@ $(LDFLAGS) $(LUAC_O) $(LUA_A) $(LIBS)

test:
	./$(LUA_T) -v

clean:
	cd vendor/Soup/soup && $(MAKE) clean && cd ../..
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CXX) $(CXXFLAGS) -MM l*.cpp

echo:
	@echo "PLAT= $(PLAT)"
	@echo "CXX= $(CXX)"
	@echo "CFLAGS= $(CXXFLAGS)"
	@echo "LDFLAGS= $(LDFLAGS)"
	@echo "LIBS= $(LIBS)"
	@echo "AR= $(AR)"
	@echo "RANLIB= $(RANLIB)"
	@echo "RM= $(RM)"
	@echo "UNAME= $(UNAME)"

# Convenience targets for popular platforms.
ALL= all

help:
	@echo "Do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"
	@echo "See doc/readme.html for complete instructions."

guess:
	@echo Guessing `$(UNAME)`
	@$(MAKE) `$(UNAME)`

AIX aix:
	$(MAKE) $(ALL) CXX="xlc" CFLAGS="-O2 -DLUA_USE_POSIX -DLUA_USE_DLOPEN" SYSLIBS="-ldl -lresolv" SYSLDFLAGS="-brtl -bexpall"

bsd:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN" SYSLIBS="-Wl,-E"

FreeBSD NetBSD OpenBSD freebsd:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX -DLUA_USE_READLINE -I/usr/include/edit" SYSLIBS="-ledit" CC="cc"

generic: $(ALL)

Linux linux:	linux-noreadline

linux-noreadline:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX" SYSLIBS="-ldl -lresolv"

linux-readline:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX -DLUA_USE_READLINE" SYSLIBS="-ldl -lresolv -lreadline"

termux:	termux-noreadline

termux-noreadline:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX" SYSLIBS="-ldl"

termux-readline:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX -DLUA_USE_READLINE" SYSLIBS="-ldl -lreadline"

Darwin macos macosx:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_MACOSX -DLUA_USE_READLINE" SYSLIBS="-lresolv -lreadline"

mingw:
	$(MAKE) "LUA_T=pluto.exe" "SYSLDFLAGS=-static-libstdc++ -municode" "SYSLIBS=-lws2_32 -lbcrypt -ldnsapi" pluto.exe
	$(MAKE) "LUAC_T=plutoc.exe" "SYSLDFLAGS=-static-libstdc++ -municode" "SYSLIBS=-lws2_32 -lbcrypt -ldnsapi" plutoc.exe

mingwdll:
	$(MAKE) "LUA_SO=pluto.dll" "SYSCFLAGS=-DLUA_BUILD_AS_DLL" "SYSLIBS=-static-libstdc++ -lws2_32 -lbcrypt -ldnsapi -ldnsapi" pluto.dll

posix:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_POSIX"

SunOS solaris:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_POSIX -DLUA_USE_DLOPEN -D_REENTRANT" SYSLIBS="-ldl -lresolv"

# Targets that do not create files (not all makes understand .PHONY).
.PHONY: all $(PLATS) help test clean default o a depend echo

# Compiler modules may use special flags.
llex.o:
	$(CXX) $(CXXFLAGS) $(CMCFLAGS) -c llex.cpp

lparser.o:
	$(CXX) $(CXXFLAGS) $(CMCFLAGS) -c lparser.cpp

lcode.o:
	$(CXX) $(CXXFLAGS) $(CMCFLAGS) -c lcode.cpp

# DO NOT DELETE

lapi.o: lapi.cpp lprefix.h lua.h luaconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h lstring.h \
 ltable.h lundump.h lvm.h
lauxlib.o: lauxlib.cpp lprefix.h lua.h luaconf.h lauxlib.h
lbaselib.o: lbaselib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lcode.o: lcode.cpp lprefix.h lua.h luaconf.h lcode.h llex.h lobject.h \
 llimits.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h \
 ldo.h lgc.h lstring.h ltable.h lvm.h
lcorolib.o: lcorolib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lctype.o: lctype.cpp lprefix.h lctype.h lua.h luaconf.h llimits.h
ldblib.o: ldblib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
ldebug.o: ldebug.cpp lprefix.h lua.h luaconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h lcode.h llex.h lopcodes.h lparser.h \
 ldebug.h ldo.h lfunc.h lstring.h lgc.h ltable.h lvm.h
ldo.o: ldo.cpp lprefix.h lua.h luaconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h lopcodes.h \
 lparser.h lstring.h ltable.h lundump.h lvm.h
ldump.o: ldump.cpp lprefix.h lua.h luaconf.h lobject.h llimits.h lstate.h \
 ltm.h lzio.h lmem.h lundump.h
lfunc.o: lfunc.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h
lgc.o: lgc.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h
linit.o: linit.cpp lprefix.h lua.h luaconf.h lualib.h lauxlib.h
liolib.o: liolib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
llex.o: llex.cpp lprefix.h lua.h luaconf.h lctype.h llimits.h ldebug.h \
 lstate.h lobject.h ltm.h lzio.h lmem.h ldo.h lgc.h llex.h lparser.h \
 lstring.h ltable.h
lmathlib.o: lmathlib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lmem.o: lmem.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h
loadlib.o: loadlib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lobject.o: lobject.cpp lprefix.h lua.h luaconf.h lctype.h llimits.h \
 ldebug.h lstate.h lobject.h ltm.h lzio.h lmem.h ldo.h lstring.h lgc.h \
 lvm.h
lopcodes.o: lopcodes.cpp lprefix.h lopcodes.h llimits.h lua.h luaconf.h
loslib.o: loslib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lparser.o: lparser.cpp lprefix.h lua.h luaconf.h lcode.h llex.h lobject.h \
 llimits.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h \
 ldo.h lfunc.h lstring.h lgc.h ltable.h
lstate.o: lstate.cpp lprefix.h lua.h luaconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h llex.h \
 lstring.h ltable.h
lstring.o: lstring.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h ldo.h lstring.h lgc.h
lstrlib.o: lstrlib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lcryptolib.o: lcryptolib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h lcryptolib.hpp
ltable.o: ltable.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h lstring.h ltable.h lvm.h
ltablib.o: ltablib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
ltm.o: ltm.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h lstring.h ltable.h lvm.h
lua.o: lua.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
luac.o: luac.cpp lprefix.h lua.h luaconf.h lauxlib.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h lopcodes.h lopnames.h lundump.h
lundump.o: lundump.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lstring.h lgc.h \
 lundump.h
lutf8lib.o: lutf8lib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lassertlib.o: lassertlib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lvector3lib.o: lvector3lib.cpp lprefix.h lua.h luaconf.h lauxlib.h lualib.h
lvm.o: lvm.cpp lprefix.h lua.h luaconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lstring.h \
 ltable.h lvm.h ljumptab.h ljumptabgcc.h
lzio.o: lzio.cpp lprefix.h lua.h luaconf.h llimits.h lmem.h lstate.h \
 lobject.h ltm.h lzio.h

# (end of Makefile)
