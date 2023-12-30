#pragma once

#include "MimeMessage.hpp"

namespace soup
{
	struct HttpResponse : public MimeMessage
	{
		uint16_t status_code;
		std::string status_text;

		using MimeMessage::MimeMessage;
	};
}
