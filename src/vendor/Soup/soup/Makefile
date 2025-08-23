CXX=g++ -std=c++17 -O3 -flto -fvisibility=hidden -fPIC
CFLAGS=-c -Wall

LIBNAME=libsoup.a
# echo $(ls *.cpp | sed 's/.cpp/.o/g')
OBJS=Asn1Identifier.o Asn1Sequence.o Bigint.o Canvas.o Capture.o CpuInfo.o Curve25519.o DefaultRngInterface.o DetachedScheduler.o HardwareRng.o HttpRequest.o HttpRequestTask.o IpAddr.o JsonArray.o JsonBool.o JsonFloat.o JsonInt.o JsonNode.o JsonNull.o JsonObject.o JsonString.o LangDesc.o LexemeParser.o MimeMessage.o Mixed.o Oid.o ParserState.o Process.o ProcessHandle.o Promise.o QrCode.o RasterFont.o Reader.o Regex.o RegexGroup.o ResolveIpAddrTask.o Rgb.o Scheduler.o SelfDeletingThread.o Server.o SharedLibrary.o Socket.o SocketAddr.o SocketTlsEncrypter.o SocketTlsHandshaker.o Task.o Thread.o TinyPngOut.o TrustStore.o Uri.o Worker.o Writer.o X509Certchain.o X509Certificate.o X509RelativeDistinguishedName.o adler32.o aes.o alloc.o base.o base32.o base64.o cat.o crc32.o crc32c.o deflate.o dnsHttpResolver.o dnsName.o dnsRawResolver.o dnsResolver.o dnsSmartResolver.o dnsUdpResolver.o dns_records.o ecc.o ffi.o filesystem.o joaat.o json.o log.o memGuard.o netConfig.o netConnectTask.o netStatus.o os.o parse_tree.o pem.o rand.o rflParser.o ripemd160.o rsa.o sha1.o sha256.o sha384.o sha512.o spaceship.o string.o time.o unicode.o urlenc.o version_compare.o xml.o Range.o dnsOsResolver.o dnsType.o

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $(LIBNAME) $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ${OBJS} $(LIBNAME)
