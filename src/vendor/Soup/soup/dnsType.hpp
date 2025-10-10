#pragma once

#include <cstdint>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum dnsType : uint16_t
	{
		DNS_NONE = 0,

		DNS_A = 1, // a host address [RFC1035]
		DNS_NS = 2, // an authoritative name server [RFC1035]
		DNS_MD = 3, // a mail destination (OBSOLETE - use MX) [RFC1035]
		DNS_MF = 4, // a mail forwarder (OBSOLETE - use MX) [RFC1035]
		DNS_CNAME = 5, // the canonical name for an alias [RFC1035]
		DNS_SOA = 6, // marks the start of a zone of authority [RFC1035]
		DNS_MB = 7, // a mailbox domain name (EXPERIMENTAL) [RFC1035]
		DNS_MG = 8, // a mail group member (EXPERIMENTAL) [RFC1035]
		DNS_MR = 9, // a mail rename domain name (EXPERIMENTAL) [RFC1035]
		DNS_NULL = 10, // a null RR (EXPERIMENTAL) [RFC1035]
		DNS_WKS = 11, // a well known service description [RFC1035]
		DNS_PTR = 12, // a domain name pointer [RFC1035]
		DNS_HINFO = 13, // host information [RFC1035]
		DNS_MINFO = 14, // mailbox or mail list information [RFC1035]
		DNS_MX = 15, // mail exchange [RFC1035]
		DNS_TXT = 16, // text strings [RFC1035]
		DNS_RP = 17, // for Responsible Person [RFC1183]
		DNS_AFSDB = 18, // for AFS Data Base location [RFC1183][RFC5864]
		DNS_X25 = 19, // for X.25 PSDN address [RFC1183]
		DNS_ISDN = 20, // for ISDN address [RFC1183]
		DNS_RT = 21, // for Route Through [RFC1183]
		DNS_NSAP = 22, // for NSAP address, NSAP style A record [RFC1706]
		DNS_NSAP_PTR = 23, // for domain name pointer, NSAP style [RFC1706]
		DNS_SIG = 24, // for security signature [RFC2536][RFC2931][RFC3110][RFC4034]
		DNS_KEY = 25, // for security key [RFC2536][RFC2539][RFC3110][RFC4034]
		DNS_PX = 26, // X.400 mail mapping information [RFC2163]
		DNS_GPOS = 27, // Geographical Position [RFC1712]
		DNS_AAAA = 28, // IP6 Address [RFC3596]
		DNS_LOC = 29, // Location Information [RFC1876]
		DNS_NXT = 30, // Next Domain (OBSOLETE) [RFC2535][RFC3755]
		DNS_EID = 31, // Endpoint Identifier [Michael_Patton][http://ana-3.lcs.mit.edu/~jnc/nimrod/dns.txt]  1995-06
		DNS_NIMLOC = 32, // Nimrod Locator [1][Michael_Patton][http://ana-3.lcs.mit.edu/~jnc/nimrod/dns.txt]  1995-06
		DNS_SRV = 33, // Server Selection [1][RFC2782]
		DNS_ATMA = 34, // ATM Address [ ATM Forum Technical Committee, "ATM Name System, V2.0", Doc ID: AF-DANS-0152.000, July 2000. Available from and held in escrow by IANA.]
		DNS_NAPTR = 35, // Naming Authority Pointer [RFC3403]
		DNS_KX = 36, // Key Exchanger [RFC2230]
		DNS_CERT = 37, // CERT [RFC4398]
		DNS_A6 = 38, // A6 (OBSOLETE - use AAAA) [RFC2874][RFC3226][RFC6563]
		DNS_DNAME = 39, // DNAME [RFC6672]
		DNS_SINK = 40, // SINK [Donald_E_Eastlake][draft-eastlake-kitchen-sink]  1997-11
		DNS_OPT = 41, // OPT [RFC3225][RFC6891]
		DNS_APL = 42, // APL [RFC3123]
		DNS_DS = 43, // Delegation Signer [RFC4034]
		DNS_SSHFP = 44, // SSH Key Fingerprint [RFC4255]
		DNS_IPSECKEY = 45, // IPSECKEY [RFC4025]
		DNS_RRSIG = 46, // RRSIG [RFC4034]
		DNS_NSEC = 47, // NSEC [RFC4034][RFC9077]
		DNS_DNSKEY = 48, // DNSKEY [RFC4034]
		DNS_DHCID = 49, // DHCID [RFC4701]
		DNS_NSEC3 = 50, // NSEC3 [RFC5155][RFC9077]
		DNS_NSEC3PARAM = 51, // NSEC3PARAM [RFC5155]
		DNS_TLSA = 52, // TLSA [RFC6698]
		DNS_SMIMEA = 53, // S/MIME cert association [RFC8162] SMIMEA/smimea-completed-template 2015-12-01
		DNS_HIP = 55, // Host Identity Protocol [RFC8005]
		DNS_NINFO = 56, // NINFO [Jim_Reid] NINFO/ninfo-completed-template 2008-01-21
		DNS_RKEY = 57, // RKEY [Jim_Reid] RKEY/rkey-completed-template 2008-01-21
		DNS_TALINK = 58, // Trust Anchor LINK [Wouter_Wijngaards] TALINK/talink-completed-template 2010-02-17
		DNS_CDS = 59, // Child DS [RFC7344] CDS/cds-completed-template 2011-06-06
		DNS_CDNSKEY = 60, // DNSKEY(s) the Child wants reflected in DS [RFC7344]  2014-06-16
		DNS_OPENPGPKEY = 61, // OpenPGP Key [RFC7929] OPENPGPKEY/openpgpkey-completed-template 2014-08-12
		DNS_CSYNC = 62, // Child-To-Parent Synchronization [RFC7477]  2015-01-27
		DNS_ZONEMD = 63, // Message Digest Over Zone Data [RFC8976] ZONEMD/zonemd-completed-template 2018-12-12
		DNS_SVCB = 64, // General Purpose Service Binding [RFC-ietf-dnsop-svcb-https-10] SVCB/svcb-completed-template 2020-06-30
		DNS_HTTPS = 65, // Service Binding type for use with HTTP [RFC-ietf-dnsop-svcb-https-10] HTTPS/https-completed-template 2020-06-30
		DNS_SPF = 99, //  [RFC7208]
		DNS_UINFO = 100, //  [IANA-Reserved]
		DNS_UID = 101, //  [IANA-Reserved]
		DNS_GID = 102, //  [IANA-Reserved]
		DNS_UNSPEC = 103, //  [IANA-Reserved]
		DNS_NID = 104, //  [RFC6742] ILNP/nid-completed-template 
		DNS_L32 = 105, //  [RFC6742] ILNP/l32-completed-template 
		DNS_L64 = 106, //  [RFC6742] ILNP/l64-completed-template 
		DNS_LP = 107, //  [RFC6742] ILNP/lp-completed-template 
		DNS_EUI48 = 108, // an EUI-48 address [RFC7043] EUI48/eui48-completed-template 2013-03-27
		DNS_EUI64 = 109, // an EUI-64 address [RFC7043] EUI64/eui64-completed-template 2013-03-27
		DNS_TKEY = 249, // Transaction Key [RFC2930]
		DNS_TSIG = 250, // Transaction Signature [RFC8945]
		DNS_IXFR = 251, // incremental transfer [RFC1995]
		DNS_AXFR = 252, // transfer of an entire zone [RFC1035][RFC5936]
		DNS_MAILB = 253, // mailbox-related RRs (MB, MG or MR) [RFC1035]
		DNS_MAILA = 254, // mail agent RRs (OBSOLETE - see MX) [RFC1035]
		DNS_ALL = 255, // A request for some or all records the server has available [RFC1035][RFC6895][RFC8482]
		DNS_URI = 256, // URI [RFC7553] URI/uri-completed-template 2011-02-22
		DNS_CAA = 257, // Certification Authority Restriction [RFC8659] CAA/caa-completed-template 2011-04-07
		DNS_AVC = 258, // Application Visibility and Control [Wolfgang_Riedel] AVC/avc-completed-template 2016-02-26
		DNS_DOA = 259, // Digital Object Architecture [draft-durand-doa-over-dns] DOA/doa-completed-template 2017-08-30
		DNS_AMTRELAY = 260, // Automatic Multicast Tunneling Relay [RFC8777] AMTRELAY/amtrelay-completed-template 2019-02-06
		DNS_TA = 32768, // DNSSEC Trust Authorities [Sam_Weiler][http://cameo.library.cmu.edu/][ Deploying DNSSEC Without a Signed Root. Technical Report 1999-19, Information Networking Institute, Carnegie Mellon University, April 2004.]  2005-12-13
		DNS_DLV = 32769, // DNSSEC Lookaside Validation (OBSOLETE) [RFC8749][RFC4431]
	};

	[[nodiscard]] extern std::string dnsTypeToString(dnsType type);
	[[nodiscard]] extern dnsType dnsTypeFromString(const char* str);
	[[nodiscard]] inline dnsType dnsTypeFromString(const std::string& str) { return dnsTypeFromString(str.c_str()); };
}
