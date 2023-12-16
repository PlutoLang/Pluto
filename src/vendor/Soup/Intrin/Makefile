CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall -msha -msse4.1

LIBNAME=libsoupintrin.a
OBJS=sha1_transform_intrin.o sha256_transform_intrin.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm ${OBJS} $(LIBNAME)

