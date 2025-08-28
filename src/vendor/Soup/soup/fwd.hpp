#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	// algos.rng.interface
	struct RngInterface;
	struct StatelessRngInterface;

	// audio
	class audPlayback;
	struct audSound;

	// crypto
	struct CertStore;
	struct RsaKeypair;
	struct RsaPrivateKey;
	class TrustStore;
	class YubikeyValidator;

	// crypto.x509
	struct X509Certchain;
	struct X509Certificate;

	// data
	struct Oid;

	// data.asn1
	struct Asn1Sequence;

	// data.container
	struct StructMap;

	// data.json
	struct JsonArray;
	struct JsonBool;
	struct JsonFloat;
	struct JsonInt;
	struct JsonObject;
	struct JsonString;
	struct JsonTreeWriter;

	// data.reflection
	class drData;

	// data.regex
	struct RegexConstraint;
	struct RegexGroup;
	struct RegexMatcher;
	struct RegexTransitionsVector;

	// data.xml
	struct PlistDict;
	struct PlistArray;
	struct PlistString;
	struct XmlMode;
	struct XmlNode;
	struct XmlTag;
	struct XmlText;

	// hardware
	class hwHid;

	// hardware.keyboard
	enum Key : uint8_t;

	// hardware.keyboard.rgb
	class kbRgbWooting;

	// io.bits
	class BitReader;
	class BitWriter;

	// io.stream
	class Reader;
	class StringReader;
	class Writer;
	class StringWriter;

	// lang.compiler
	struct astBlock;
	struct astNode;
	class LangDesc;
	struct Lexeme;
	struct LexemeParser;
	class ParserState;
	struct Token;

	// lang.compiler.ir
	struct irFunction;

	// lang.reflection
	struct rflFunc;
	struct rflStruct;
	struct rflType;
	struct rflVar;

	// ling.chatbot
	struct cbCmd;

	// math
	class Bigint;
	struct Vector2;
	struct Vector3;

	// math.3d
	class Matrix;
	struct Mesh;
	struct Poly;
	class Quaternion;
	struct Ray;

	// math.3d.geometry
	struct gmBoxCorners;

	// math.3d.scene
	struct Scene;

	// mem
	class Pattern;
	struct CompiletimePatternWithOptBytesBase;
	class Pointer;
	class Range;
	template <typename T> class SharedPtr;
	template <typename T> class UniquePtr;
	template <class T> class WeakRef;

	// mem.allocraii
	struct AllocRaiiLocalBase;
	struct AllocRaiiRemote;
	struct AllocRaiiVirtual;

	// mem.vft
	struct memVft;
	struct RttiObject;

	// misc.chess
	struct ChessCoordinate;

	// net
	class IpAddr;
	struct netConfig;
	class netIntel;
	enum netStatus : uint8_t;
	class Server;
	struct ServerService;
	struct ServerServiceUdp;
	class ServerWebService;
	class Socket;
	struct SocketAddr;

	// net.dns.resolver
	struct dnsResolver;
	struct dnsName;
	struct dnsHttpResolver;

	// net.tls
	class SocketTlsHandshaker;
	struct TlsClientHello;
	struct TlsExtAlpn;

	// net.web
	class HttpRequest;
	class HttpRequestTask;
	struct HttpResponse;
	struct Uri;

	// net.web.websocket
	struct WebSocketMessage;

	// os
	struct HandleRaii;
	class Module;
	enum MouseButton : uint8_t;
	class ProcessHandle;
	class Thread;
	struct Window;

	// task
	class Capture;
	class DetachedScheduler;
	class PromiseBase;
	template <typename T> class Promise;
	class Scheduler;

	// util
	class Mixed;

	// vis
	struct BCanvas;
	class Canvas;
	struct FormattedText;
	class QrCode;
	struct RasterFont;
	union Rgb;

	// vis.layout
	struct lyoContainer;
	struct lyoDocument;
	struct lyoElement;
	struct lyoFlatDocument;
	struct lyoTextElement;

	// vis.render
	struct RenderTarget;

	// vis.ui.conui
	struct ConuiApp;
	struct ConuiDiv;

	// vis.ui.editor
	struct Editor;
	struct EditorText;
}
