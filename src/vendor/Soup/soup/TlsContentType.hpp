#pragma once

#include "type.hpp"

namespace soup
{
	struct TlsContentType
	{
		enum _ : TlsContentType_t
		{
			change_cipher_spec = 20,
			alert = 21,
			handshake = 22,
			application_data = 23,
		};
	};
}
