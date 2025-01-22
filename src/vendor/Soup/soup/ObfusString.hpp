#pragma once

#include <algorithm> // swap
#include <string>

#include "base.hpp"
#include "LcgRng.hpp"
#include "rand.hpp"
#include "string.hpp"
#include "StringLiteral.hpp"

#define SOUP_ASSERT_OBF(x, msg) SOUP_IF_UNLIKELY (!(x)) { soup::ObfusString str(msg); ::soup::throwAssertionFailed(str.c_str()); }

NAMESPACE_SOUP
{
#pragma pack(push, 1)
	template <size_t Size>
	class ObfusString
	{
	public:
		static constexpr size_t Len = Size - 1;

	private:
		// seed serves as null-terminator as it's set to 0 after deobfuscation
		char m_data[Len];
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

			// copy input & apply rot13
			for (size_t i = 0; i != Len; ++i)
			{
				m_data[i] = string::rot13(in[i]);
			}

			if (Len < 40'000)
			{
				// mirror
				for (size_t i = 0, j = Len - 1; i != Len / 2; ++i, --j)
				{
					std::swap(m_data[i], m_data[j]);
				}
			}

			// flip bits
			for (size_t i = 0; i != Len; ++i)
			{
				const auto m = i % 8;
				if (m == 0)
				{
					rng.skip();
				}
				m_data[i] ^= rng.state >> (m * 8);
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

			// flip bits
			for (size_t i = 0; i != Len; ++i)
			{
				const auto m = i % 8;
				if (m == 0)
				{
					rng.skip();
				}
				m_data[i] ^= rng.state >> (m * 8);
			}

			if constexpr (Len < 40'000)
			{
				// mirror
				for (size_t i = 0, j = Len - 1; i != Len / 2; ++i, --j)
				{
					std::swap(m_data[i], m_data[j]);
				}
			}

			// rot13
			for (size_t i = 0; i != Len; ++i)
			{
				m_data[i] = string::rot13(m_data[i]);
			}
		}

	public:
		[[nodiscard]] std::string str() SOUP_EXCAL
		{
			runtime_access();
			return std::string(m_data, Len);
		}

		[[nodiscard]] operator std::string() SOUP_EXCAL
		{
			return str();
		}

		[[nodiscard]] const char* c_str() noexcept
		{
			runtime_access();
			return m_data;
		}

		[[nodiscard]] const char* data() noexcept
		{
			runtime_access();
			return m_data;
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

		[[nodiscard]] constexpr size_t size() const noexcept
		{
			return Len;
		}

		[[nodiscard]] constexpr size_t length() const noexcept
		{
			return Len;
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
			return ObfusString<Str.size() + 1>(Str.m_data);
		}
	}
#endif
}
