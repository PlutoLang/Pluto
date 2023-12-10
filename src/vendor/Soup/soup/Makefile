CC=g++
CFLAGS=-c -Wall

LIBNAME=libsoup.a
OBJS=base32.o base64.o JsonArray.o JsonBool.o json.o JsonFloat.o JsonInt.o JsonNode.o JsonNull.o JsonObject.o JsonString.o spaceship.o unicode.o urlenc.o version_compare.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm ${OBJS} $(LIBNAME)

