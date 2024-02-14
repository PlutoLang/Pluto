#pragma once

#include <algorithm> // swap
#include <string>

#include "base.hpp"
#include "LcgRng.hpp"
#include "rand.hpp"
#include "string.hpp"
#include "StringLiteral.hpp"

namespace soup
{
#pragma pack(push, 1)
	template <size_t Size>
	class ObfusString
	{
	public:
		static constexpr size_t Len = Size - 1;

	private:
		// seed serves as null-terminator as it's set to 0 after deobfuscation
		char data[Len];
		uint32_t seed;

	public:
		SOUP_CONSTEVAL ObfusString(const char(&in)[Size])
		{
			initialise(in);
		}

		template <typename T = char>
		SOUP_CONSTEVAL ObfusString(const T* in)
		{
			initialise(in);
		}

	private:
		SOUP_CONSTEVAL void initialise(const char* in)
		{
			seed = rand.getConstexprSeed(Len);
			LcgRng rng(seed);

			// rot13
			for (size_t i = 0; i != Len; ++i)
			{
				data[i] = string::rot13(in[i]);
			}

			// flip bits
			for (size_t i = 0; i != Len; ++i)
			{
				data[i] ^= rng.generateByte();
			}

			// mirror
			for (size_t i = 0, j = Len - 1; i != Len / 2; ++i, --j)
			{
				std::swap(data[i], data[j]);
			}
		}

		SOUP_NOINLINE void runtime_access() noexcept
		{
			if (seed == 0)
			{
				return;
			}
			LcgRng rng(seed);
			seed = 0;

			// mirror
			for (size_t i = 0, j = Len - 1; i != Len / 2; ++i, --j)
			{
				std::swap(data[i], data[j]);
			}

			// flip bits
			for (size_t i = 0; i != Len; ++i)
			{
				data[i] ^= rng.generateByte();
			}

			// rot13
			for (size_t i = 0; i != Len; ++i)
			{
				data[i] = string::rot13(data[i]);
			}
		}

	public:
		[[nodiscard]] std::string str() noexcept
		{
			runtime_access();
			return std::string(data, Len);
		}

		[[nodiscard]] operator std::string() noexcept
		{
			return str();
		}

		[[nodiscard]] const char* c_str() noexcept
		{
			runtime_access();
			return data;
		}

		[[nodiscard]] operator const char* () noexcept
		{
			return c_str();
		}

		[[nodiscard]] bool operator==(const std::string& b) noexcept
		{
			return str() == b;
		}

		[[nodiscard]] bool operator!=(const std::string& b) noexcept
		{
			return !operator==(b);
		}

		template <size_t BLen>
		[[nodiscard]] bool operator==(const ObfusString<BLen>& b) noexcept
		{
			return str() == b.str();
		}

		template <size_t BLen>
		[[nodiscard]] bool operator!=(const ObfusString<BLen>& b) noexcept
		{
			return !operator==(b);
		}

		[[nodiscard]] bool operator==(const char* b) noexcept
		{
			return strcmp(c_str(), b) == 0;
		}

		[[nodiscard]] bool operator!=(const char* b) noexcept
		{
			return !operator==(b);
		}

		friend std::ostream& operator<<(std::ostream& os, ObfusString& str)
		{
			os << str.str();
			return os;
		}
	};
	static_assert(sizeof(ObfusString<3>) == 2 + 4);
#pragma pack(pop)

#if SOUP_CPP20
	namespace literals
	{
		template <StringLiteral Str>
		SOUP_CONSTEVAL auto operator ""_obfus()
		{
			return ObfusString<Str.size()>(Str.data);
		}
	}
#endif
}
