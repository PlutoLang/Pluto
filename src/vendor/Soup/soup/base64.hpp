#pragma once

#include <cstring> // memcpy
#include <string>

#include "base.hpp" // SOUP_EXCAL

namespace soup
{
	struct base64
	{
		[[nodiscard]] static std::string encode(const char* data, const bool pad = true) SOUP_EXCAL;
		[[nodiscard]] static std::string encode(const std::string& data, const bool pad = true) SOUP_EXCAL;
		template <typename T>
		[[nodiscard]] static std::string encode(const T& data, const bool pad = true) SOUP_EXCAL
		{
			return encode(&data, pad);
		}
		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad = true) SOUP_EXCAL;

		[[nodiscard]] static std::string urlEncode(const char* data, const bool pad = false) SOUP_EXCAL;
		[[nodiscard]] static std::string urlEncode(const std::string& data, const bool pad = false) SOUP_EXCAL;
		[[nodiscard]] static std::string urlEncode(const char* const data, const size_t size, const bool pad = false) SOUP_EXCAL;

		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad, const char* table) SOUP_EXCAL;

		[[nodiscard]] static std::string decode(std::string enc) SOUP_EXCAL;
		[[nodiscard]] static std::string urlDecode(std::string enc) SOUP_EXCAL;
		[[nodiscard]] static std::string decode(std::string&& enc, const unsigned char* table) SOUP_EXCAL;

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
