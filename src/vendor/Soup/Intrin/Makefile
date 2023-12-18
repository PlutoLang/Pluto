CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall -msha -msse4.1

LIBNAME=libsoupintrin.a
OBJS=sha256_transform_intrin.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm ${OBJS} $(LIBNAME)

