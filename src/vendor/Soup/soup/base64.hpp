#pragma once

#include <cstring> // memcpy
#include <string>

#include "base.hpp" // SOUP_EXCAL

NAMESPACE_SOUP
{
	struct base64
	{
		[[nodiscard]] static constexpr size_t getEncodedSize(size_t size) noexcept
		{
			return 4 * ((size + 2) / 3);
		}

		[[nodiscard]] static constexpr size_t getPadBytes(size_t size) noexcept
		{
			return (3 - (size % 3)) % 3;
		}

		[[nodiscard]] static constexpr size_t getEncodedSize(size_t size, bool pad) noexcept
		{
			size_t enc_size = getEncodedSize(size);
			if (!pad)
			{
				enc_size -= getPadBytes(size);
			}
			return enc_size;
		}

		[[nodiscard]] static std::string encode(const char* data, const bool pad = true) SOUP_EXCAL;
		[[nodiscard]] static std::string encode(const std::string& data, const bool pad = true) SOUP_EXCAL;
		template <typename T>
		[[nodiscard]] static std::string encode(const T& data, const bool pad = true) SOUP_EXCAL
		{
			return encode(&data, pad);
		}
		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad = true) SOUP_EXCAL;
		static void encode(char* out, const char* const data, const size_t size, const bool pad) noexcept;

		[[nodiscard]] static std::string urlEncode(const char* data, const bool pad = false) SOUP_EXCAL;
		[[nodiscard]] static std::string urlEncode(const std::string& data, const bool pad = false) SOUP_EXCAL;
		[[nodiscard]] static std::string urlEncode(const char* const data, const size_t size, const bool pad = false) SOUP_EXCAL;
		static void urlEncode(char* out, const char* const data, const size_t size, const bool pad) noexcept;

		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad, const char table[64]) SOUP_EXCAL;
		static void encode(char* out, const char* data, const size_t size, const bool pad, const char table[64]) noexcept;

		[[nodiscard]] static size_t getDecodedSize(const char* data, size_t size) noexcept;
		[[nodiscard]] static std::string decode(const std::string& enc) SOUP_EXCAL;
		static void decode(char* out, const char* data, size_t size) noexcept;
		[[nodiscard]] static std::string urlDecode(const std::string& enc) SOUP_EXCAL;
		static void urlDecode(char* out, const char* data, size_t size) noexcept;
		[[nodiscard]] static std::string decode(const std::string& enc, const unsigned char table[256]) SOUP_EXCAL;
		static void decode(char* out, const char* data, size_t size, const unsigned char table[256]) noexcept;

		template <typename T>
		static bool decode(T& out, std::string enc) SOUP_EXCAL
		{
			std::string tmp = decode(std::move(enc));
			if (tmp.size() != sizeof(T))
			{
				return false;
			}
			memcpy(&out, tmp.data(), sizeof(T));
			return true;
		}
	};
}
