#pragma once

#include <cstdint>
#include <string>

NAMESPACE_SOUP
{
	struct rflType
	{
		enum AccessType : uint8_t
		{
			DIRECT = 0,
			POINTER,
			REFERENCE,
			RVALUE_REFERENCE,
		};

		std::string name;
		AccessType at;

		[[nodiscard]] std::string toString() const
		{
			std::string str = name;
			const char* access_type_strings[] = { "", "*", "&", "&&" };
			str.append(access_type_strings[at]);
			return str;
		}

		[[nodiscard]] size_t getSize() const noexcept
		{
			if (at == DIRECT)
			{
				if (name == "bool") { return sizeof(bool); }
				if (name == "char") { return sizeof(char); }
				if (name == "int8_t") { return sizeof(int8_t); }
				if (name == "uint8_t") { return sizeof(uint8_t); }
				if (name == "short") { return sizeof(short); }
				if (name == "int16_t") { return sizeof(int16_t); }
				if (name == "uint16_t") { return sizeof(uint16_t); }
				if (name == "int") { return sizeof(int); }
				if (name == "int32_t") { return sizeof(int32_t); }
				if (name == "uint32_t") { return sizeof(uint32_t); }
				if (name == "int64_t") { return sizeof(int64_t); }
				if (name == "uint64_t") { return sizeof(uint64_t); }
				if (name == "size_t") { return sizeof(size_t); }
				if (name == "float") { return sizeof(float); }
				if (name == "double") { return sizeof(double); }
			}
			return sizeof(size_t);
		}
	};
}
