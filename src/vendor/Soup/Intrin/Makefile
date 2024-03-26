CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall

ifeq ($(shell uname -m),x86_64)
	CFLAGS+= -maes -mpclmul -mrdrnd -mrdseed -msha -msse4.1
endif

LIBNAME=libsoupintrin.a
OBJS=aes_helper.o crc32_pclmul.o hardware_rng.o sha1_transform_intrin.o sha256_transform_intrin.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ${OBJS} $(LIBNAME)
