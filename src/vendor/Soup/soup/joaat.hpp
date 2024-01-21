#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "base.hpp"

namespace soup
{
	struct joaat
	{
		static constexpr uint32_t INITIAL = 0;

		[[nodiscard]] static uint32_t hash(const std::string& str, uint32_t initial = 0) noexcept;
		[[nodiscard]] static uint32_t concat(uint32_t val, const std::string& str) noexcept;

		[[nodiscard]] static constexpr uint32_t hash(const char* str) noexcept
		{
			uint32_t val = 0;
			while (*str)
			{
				val += (uint8_t)*(str++);
				val += (val << 10);
				val ^= (val >> 6);
			}
			val += (val << 3);
			val ^= (val >> 11);
			val += (val << 15);
			return val;
		}

		[[nodiscard]] static SOUP_CONSTEVAL uint32_t compileTimeHash(const char* str) noexcept
		{
			return hash(str);
		}

		[[nodiscard]] static constexpr uint32_t hash(const char* str, uint32_t initial) noexcept
		{
			uint32_t val = initial;
			while (*str)
			{
				val += (uint8_t)*(str++);
				val += (val << 10);
				val ^= (val >> 6);
			}
			val += (val << 3);
			val ^= (val >> 11);
			val += (val << 15);
			return val;
		}

		[[nodiscard]] static constexpr uint32_t hashRange(const char* data, size_t size, uint32_t initial = 0) noexcept
		{
			uint32_t val = partial(data, size, initial);
			finalise(val);
			return val;
		}

		[[nodiscard]] static constexpr uint32_t partial(const char* data, size_t size, uint32_t initial = 0) noexcept
		{
			/*uint32_t val = initial;
			while (size-- != 0)
			{
				val += *(data++);
				val += (val << 10);
				val ^= (val >> 6);
			}
			return val;*/

			size_t v3 = 0;
			uint32_t result = initial;
			int v5 = 0;
			for (; v3 < size; result = ((uint32_t)(1025 * (v5 + result)) >> 6) ^ (1025 * (v5 + result)))
			{
				v5 = data[v3++];
			}
			return result;
		}

		static constexpr void finalise(uint32_t& val) noexcept
		{
			/*val += (val << 3);
			val ^= (val >> 11);
			val += (val << 15);*/

			val = (0x8001 * (((uint32_t)(9 * val) >> 11) ^ (9 * val)));
		}

		static void undo_finalise(uint32_t& val) noexcept
		{
			val *= 0x3FFF8001; // inverse of val += (val << 15);
			val ^= (val >> 11) ^ (val >> 22);
			val *= 0x38E38E39; // inverse of val += (val << 3);
		}

		static void undo_partial(uint32_t& val) noexcept
		{
			val ^= (val >> 6) ^ (val >> 12) ^ (val >> 18) ^ (val >> 24) ^ (val >> 30);
			val *= 0xC00FFC01; // inverse of val += (val << 10);
		}

		[[nodiscard]] static uint32_t undo_partial(uint32_t val, char c) noexcept
		{
			val -= c;
			undo_partial(val);
			return val;
		}

		[[nodiscard]] static uint32_t deriveInitial(uint32_t val, const std::string& str); // deriveInitial(hash("ab"), "b") == hash("a")
		[[nodiscard]] static uint32_t deriveInitialNoFinalise(uint32_t val, const std::string& str);

		[[nodiscard]] static std::optional<std::string> reverse_short_key(uint32_t val); // If the input to joaat is 0..3 characters, this will reverse the hash.

		[[nodiscard]] static std::string collide(uint32_t val, const char* prefix = ""); // Can take quite a while to complete
	};
}
