CXX=g++ -std=c++17 -O3 -flto -fvisibility=hidden -fPIC
CFLAGS=-c -Wall

ifeq ($(shell uname -m),x86_64)
	CFLAGS+= -maes -mpclmul -mrdrnd -mrdseed -msha -msse4.1
endif

ifeq ($(shell uname -m),aarch64)
	CFLAGS+= -march=armv8-a+crypto+crc
endif
# For people who think different
ifeq ($(shell uname -m),arm64)
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
