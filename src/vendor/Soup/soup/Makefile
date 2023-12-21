CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall -DSOUP_USE_INTRIN

LIBNAME=libsoup.a
OBJS=base.o base32.o base64.o cat.o CpuInfo.o JsonArray.o JsonBool.o json.o JsonFloat.o JsonInt.o JsonNode.o JsonNull.o JsonObject.o JsonString.o sha1.o sha256.o spaceship.o unicode.o urlenc.o version_compare.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm ${OBJS} $(LIBNAME)

