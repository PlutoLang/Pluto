CXX=g++ -std=c++17 -O3 -fvisibility=hidden -flto
CFLAGS=-c -Wall -DSOUP_USE_INTRIN

LIBNAME=libsoup.a
# echo $(ls *.cpp | sed 's/.cpp/.o/g')
OBJS=adler32.o aes.o Asn1Identifier.o Asn1Sequence.o base32.o base64.o base.o Bigint.o Capture.o cat.o CpuInfo.o crc32.o Curve25519.o deflate.o DetachedScheduler.o dnsHttpResolver.o dnsName.o dnsRawResolver.o dnsSmartResolver.o dnsUdpResolver.o dns_records.o dnsResolver.o ecc.o HttpRequest.o HttpRequestTask.o IpAddr.o joaat.o JsonArray.o JsonBool.o json.o JsonFloat.o JsonInt.o JsonNode.o JsonNull.o JsonObject.o JsonString.o log.o MimeMessage.o netConfig.o netConnectTask.o netStatus.o Oid.o os.o pem.o Promise.o rand.o rsa.o Scheduler.o SelfDeletingThread.o sha1.o sha256.o Socket.o SocketTlsEncrypter.o SocketTlsHandshaker.o spaceship.o string.o Task.o Thread.o time.o TransientToken.o TrustStore.o unicode.o Uri.o urlenc.o version_compare.o Worker.o X509Certchain.o X509Certificate.o X509RelativeDistinguishedName.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ${OBJS} $(LIBNAME)
