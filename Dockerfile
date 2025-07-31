FROM alpine:3.22.1

RUN apk add --no-cache php clang lld

COPY scripts /pluto/scripts
COPY src /pluto/src

WORKDIR /pluto
RUN php scripts/compile.php clang
RUN php scripts/link_pluto.php clang \
 && php scripts/link_plutoc.php clang \
 && ln -s /pluto/src/pluto /usr/local/bin/pluto \
 && ln -s /pluto/src/plutoc /usr/local/bin/plutoc

# Now you can use 'pluto' and 'plutoc' in this container.

RUN pluto -e "print [[Testing. This layer should not take up any space.]]"
