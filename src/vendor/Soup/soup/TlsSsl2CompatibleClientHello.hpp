#pragma once

#include "Reader.hpp"
#include "TlsProtocolVersion.hpp"

NAMESPACE_SOUP
{
	struct TlsSsl2CompatibleClientHello
	{
		uint16_t msg_length;
		uint8_t msg_type;
		TlsProtocolVersion version;
		std::vector<uint32_t> cipher_suites{};
		uint16_t session_id_length;
		std::string challenge;

		bool read(Reader& r)
		{
			SOUP_RETHROW_FALSE(r.u16_be(msg_length));
			SOUP_RETHROW_FALSE(r.u8(msg_type));
			SOUP_RETHROW_FALSE(version.io(r));
			uint16_t cipher_spec_length, challenge_length;
			SOUP_RETHROW_FALSE(r.u16_be(cipher_spec_length));
			SOUP_RETHROW_FALSE(r.u16_be(session_id_length));
			SOUP_RETHROW_FALSE(r.u16_be(challenge_length));
			cipher_suites.clear();
			cipher_suites.reserve(cipher_spec_length / 3);
			while (cipher_spec_length >= 3)
			{
				SOUP_RETHROW_FALSE(r.u24_be(cipher_suites.emplace_back()));
				cipher_spec_length -= 3;
			}
			r.skip(session_id_length);
			r.str(challenge_length, challenge);
			return true;
		}
	};
}
