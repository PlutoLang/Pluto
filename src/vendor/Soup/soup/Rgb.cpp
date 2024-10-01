#include "Rgb.hpp"

#include "joaat.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	const Rgb Rgb::BLACK{ 0, 0, 0 };
	const Rgb Rgb::WHITE{ 255, 255, 255 };
	const Rgb Rgb::RED{ 255, 0, 0 };
	const Rgb Rgb::YELLOW{ 255, 255, 0 };
	const Rgb Rgb::GREEN{ 0, 255, 0 };
	const Rgb Rgb::BLUE{ 0, 0, 255 };
	const Rgb Rgb::MAGENTA{ 255, 0, 255 };
	const Rgb Rgb::GREY{ 128, 128, 128 };
	const Rgb Rgb::LIGHTGREY{ 211, 211, 211 };

	Rgb Rgb::fromHex(std::string hex)
	{
		expandHex(hex);
		static_assert(sizeof(unsigned long) >= sizeof(uint32_t));
		return Rgb(std::stoul(hex, nullptr, 16));
	}

	void Rgb::expandHex(std::string& hex)
	{
		if (hex.at(0) == '#')
		{
			hex.erase(0, 1);
		}
		if (hex.size() == 3)
		{
			hex = std::move(std::string(2, hex.at(0)).append(2, hex.at(1)).append(2, hex.at(2)));
		}
	}

	std::string Rgb::toHex() const
	{
		if (((r >> 4) == (r & 0xF))
			&& ((g >> 4) == (g & 0xF))
			&& ((b >> 4) == (b & 0xF))
			)
		{
			std::string str = string::hex(r & 0xF);
			str.append(string::hex(g & 0xF));
			str.append(string::hex(b & 0xF));
			return str;
		}
		std::string str = string::lpad(string::hex(r), 2, '0');
		str.append(string::lpad(string::hex(g), 2, '0'));
		str.append(string::lpad(string::hex(b), 2, '0'));
		return str;
	}

	Rgb Rgb::fromHsv(double hue, double saturation, double value)
	{
		// Adapted from https://stackoverflow.com/a/6930407/4796321

		double r, g, b;
		if (saturation <= 0.0) // < is bogus, just shuts up warnings
		{
			r = value;
			g = value;
			b = value;
		}
		else
		{
			while (hue >= 360.0)
			{
				hue -= 360.0;
			}
			hue /= 60.0;
			long i = (long)hue;
			double ff = hue - i;
			double p = value * (1.0 - saturation);
			double q = value * (1.0 - (saturation * ff));
			double t = value * (1.0 - (saturation * (1.0 - ff)));

			switch (i)
			{
			case 0:
				r = value;
				g = (float)t;
				b = (float)p;
				break;
			case 1:
				r = (float)q;
				g = value;
				b = (float)p;
				break;
			case 2:
				r = (float)p;
				g = value;
				b = (float)t;
				break;

			case 3:
				r = (float)p;
				g = (float)q;
				b = value;
				break;
			case 4:
				r = (float)t;
				g = (float)p;
				b = value;
				break;
			case 5:
			default:
				r = value;
				g = (float)p;
				b = (float)q;
				break;
			}
		}

		return Rgb(r * 255, g * 255, b * 255);
	}

	std::optional<Rgb> Rgb::fromName(const std::string& name)
	{
		switch (joaat::hash(name))
		{
		case joaat::hash("black"): return BLACK;
		case joaat::hash("white"): return WHITE;
		case joaat::hash("red"): return RED;
		case joaat::hash("yellow"): return YELLOW;
		case joaat::hash("green"): return GREEN;
		case joaat::hash("blue"): return BLUE;
		case joaat::hash("magenta"): return MAGENTA;
		case joaat::hash("grey"): case joaat::hash("gray"): return GREY;
		case joaat::hash("lightgrey"): case joaat::hash("lightgray"): return LIGHTGREY;
		}
		return std::nullopt;
	}

	double Rgb::distance(const Rgb& e2) const noexcept
	{
		const Rgb& e1 = *this;
#if false
		// RGB Manhattan Distance, quick but dirty
		return abs(e1.r - e2.r) + abs(e1.g - e2.g) + abs(e1.b - e2.b);
#else
		// https://stackoverflow.com/a/9085524
		long rmean = ((long)e1.r + (long)e2.r) / 2;
		long r = (long)e1.r - (long)e2.r;
		long g = (long)e1.g - (long)e2.g;
		long b = (long)e1.b - (long)e2.b;
		return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
#endif
	}

	double Rgb::similarity(const Rgb& e2) const noexcept
	{
		return 1.0 - (distance(e2) / 765.0);
	}
}
