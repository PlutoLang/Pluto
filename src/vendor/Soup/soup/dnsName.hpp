#pragma once

#include "Packet.hpp"

NAMESPACE_SOUP
{
	SOUP_PACKET(dnsName)
	{
		std::vector<std::string> name;
		u16 ptr = 0;

		SOUP_PACKET_IO(s)
		{
			SOUP_IF_ISREAD
			{
				name.clear();
				while (true)
				{
					uint8_t len;
					if (!s.u8(len))
					{
						return false;
					}
					if ((len >> 6) & 0b11)
					{
						ptr = (len & 0b111111);
						ptr <<= 8;
						if (!s.u8(len))
						{
							return false;
						}
						ptr |= len;
						break;
					}
					std::string entry;
					if (!s.str(len, entry))
					{
						return false;
					}
					if (entry.empty())
					{
						break;
					}
					name.emplace_back(std::move(entry));
				}
			}
			SOUP_ELSEIF_ISWRITE
			{
				for (const auto& entry : name)
				{
					if (!s.template str_lp<u8_t>(entry))
					{
						return false;
					}
				}
				if (ptr != 0)
				{
					uint16_t data = ptr;
					data |= 0xC000;
					return s.u16_be(data);
				}
				else
				{
					uint8_t root = 0;
					return s.u8(root);
				}
			}
			return true;
		}

		[[nodiscard]] std::vector<std::string> resolve(const std::string& data, unsigned int max_recursions = 20) const;
	};
}
