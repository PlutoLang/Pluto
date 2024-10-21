#pragma once

#include <vector>

#include "rflMember.hpp"

NAMESPACE_SOUP
{
	struct rflStruct
	{
		std::string name;
		std::vector<rflMember> members;

		[[nodiscard]] std::string toString() const noexcept
		{
			std::string str = "struct";
			if (!name.empty())
			{
				str.push_back(' ');
				str.append(name);
			}
			str.append("\n{\n");
			for (const auto& member : members)
			{
				str.push_back('\t');
				str.append(member.toString());
				str.append(";\n");
			}
			str.append("};\n");
			return str;
		}

		[[nodiscard]] size_t getOffsetOf(const std::string& name) const noexcept
		{
			size_t offset = 0;
			for (const auto& mem : members)
			{
				const auto size = mem.type.getSize();
				auto alignment = size;
				if (alignment > sizeof(size_t))
				{
					alignment = sizeof(size_t);
				}
				if ((offset % alignment) != 0)
				{
					offset += (alignment - (offset % alignment));
				}
				if (mem.name == name)
				{
					return offset;
				}
				offset += size;
			}
			return -1;
		}

		[[nodiscard]] size_t getSize() const noexcept
		{
			size_t my_size = 0;
			size_t my_alignment = 1;
			for (const auto& mem : members)
			{
				const auto size = mem.type.getSize();
				auto alignment = size;
				if (alignment > sizeof(size_t))
				{
					alignment = sizeof(size_t);
				}
				if (alignment > my_alignment)
				{
					my_alignment = alignment;
				}
				if ((my_size % alignment) != 0)
				{
					my_size += (alignment - (my_size % alignment));
				}
				my_size += size;
			}
			if ((my_size % my_alignment) != 0)
			{
				my_size += (my_alignment - (my_size % my_alignment));
			}
			return my_size;
		}
	};
}
