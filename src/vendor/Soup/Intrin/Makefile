CXX=g++ -std=c++17 -O3 -flto -fvisibility=hidden -fPIC
CFLAGS=-c -Wall

ifneq (, $(filter $(shell uname -m),x84_64 i686 i386))
	CFLAGS+= -maes -mpclmul -mrdrnd -mrdseed -msha -msse4.1
endif
ifeq ($(shell uname -m),aarch64)
	CFLAGS+= -march=armv8-a+crypto+crc
endif

LIBNAME=libsoupintrin.a
OBJS=aes_helper.o crc32_intrin.o hardware_rng.o sha1_transform.o sha256_transform.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ${OBJS} $(LIBNAME)
