CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall

ifeq ($(shell uname -m),x86_64)
	CFLAGS+= -msha -msse4.1
endif

LIBNAME=libsoupintrin.a
OBJS=sha256_transform_intrin.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ${OBJS} $(LIBNAME)
