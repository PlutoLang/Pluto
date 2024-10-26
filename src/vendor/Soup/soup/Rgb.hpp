#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "math.hpp"

#undef BLACK
#undef WHITE
#undef RED
#undef YELLOW
#undef GREEN
#undef BLUE
#undef MAGENTA
#undef GREY
#undef LIGHTGREY

NAMESPACE_SOUP
{
	union Rgb
	{
		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		uint8_t arr[3];

		static const Rgb BLACK;
		static const Rgb WHITE;
		static const Rgb RED;
		static const Rgb YELLOW;
		static const Rgb GREEN;
		static const Rgb BLUE;
		static const Rgb MAGENTA;
		static const Rgb GREY;
		static const Rgb LIGHTGREY;

		constexpr Rgb() noexcept
			: r(0), g(0), b(0)
		{
		}

		template <typename TR, typename TG, typename TB>
		constexpr Rgb(TR r, TG g, TB b) noexcept
			: r((uint8_t)r), g((uint8_t)g), b((uint8_t)b)
		{
		}

		constexpr Rgb(uint32_t val) noexcept
			: r(val >> 16), g((val >> 8) & 0xFF), b(val & 0xFF)
		{
		}

		[[nodiscard]] constexpr bool operator==(const Rgb& c) const noexcept
		{
			return r == c.r && g == c.g && b == c.b;
		}

		[[nodiscard]] constexpr bool operator!=(const Rgb& c) const noexcept
		{
			return !operator==(c);
		}

		[[nodiscard]] constexpr uint32_t toInt() const noexcept
		{
			return (r << 16) | (g << 8) | b;
		}

		[[nodiscard]] static Rgb fromHex(std::string hex);
		static void expandHex(std::string& hex);
		[[nodiscard]] std::string toHex() const;

		// hue: 0-360, saturation: 0-1, value: 0-1
		[[nodiscard]] static Rgb fromHsv(double hue, double saturation, double value);

		[[nodiscard]] static std::optional<Rgb> fromName(const std::string& name);

		[[nodiscard]] static Rgb lerp(Rgb a, Rgb b, float t)
		{
			return Rgb{
				soup::lerp<uint8_t>(a.r, b.r, t),
				soup::lerp<uint8_t>(a.g, b.g, t),
				soup::lerp<uint8_t>(a.b, b.b, t),
			};
		}

		[[nodiscard]] uint8_t& operator[](unsigned int i)
		{
			return arr[i];
		}

		[[nodiscard]] const uint8_t& operator[](unsigned int i) const
		{
			return arr[i];
		}

		[[nodiscard]] double distance(const Rgb& e2) const noexcept;
		[[nodiscard]] double similarity(const Rgb& e2) const noexcept;
		
		[[nodiscard]] Rgb invert() const noexcept
		{
			return Rgb(~r, ~g, ~b);
		}
	};
}
