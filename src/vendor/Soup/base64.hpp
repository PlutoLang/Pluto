#pragma once

#include <cstdint>
#include <cstring> // memcpy
#include <string>

namespace soup
{
	// Adapted from https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594

	struct base64
	{
		[[nodiscard]] static std::string encode(const char* data, const bool pad = true) noexcept;
		[[nodiscard]] static std::string encode(const std::string& data, const bool pad = true) noexcept;
		template <typename T>
		[[nodiscard]] static std::string encode(const T& data, const bool pad = true) noexcept
		{
			return encode(&data, pad);
		}
		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad = true) noexcept;

		[[nodiscard]] static std::string urlEncode(const char* data, const bool pad = false) noexcept;
		[[nodiscard]] static std::string urlEncode(const std::string& data, const bool pad = false) noexcept;
		[[nodiscard]] static std::string urlEncode(const char* const data, const size_t size, const bool pad = false) noexcept;

		[[nodiscard]] static std::string encode(const char* const data, const size_t size, const bool pad, const char* table) noexcept;

		[[nodiscard]] static std::string decode(std::string enc);
		[[nodiscard]] static std::string urlDecode(std::string enc);
		[[nodiscard]] static std::string decode(std::string&& enc, const unsigned char* table);

		template <typename T>
		static bool decode(T& out, std::string enc) noexcept
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
