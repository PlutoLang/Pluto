#include "Canvas.hpp"

#include "base.hpp"

#include "console.hpp"
#include "RasterFont.hpp"
#include "Reader.hpp"
#include "StringWriter.hpp"
#include "TinyPngOut.hpp"
#include "unicode.hpp"

NAMESPACE_SOUP
{
	void Canvas::fill(const Rgb colour)
	{
		for (auto& p : pixels)
		{
			p = colour;
		}
	}

	void Canvas::set(unsigned int x, unsigned int y, Rgb colour) noexcept
	{
		SOUP_IF_LIKELY (x < width && y < height)
		{
			pixels.at(x + (y * width)) = colour;
		}
	}

	Rgb Canvas::get(unsigned int x, unsigned int y) const
	{
		return pixels.at(x + (y * width));
	}

	Rgb& Canvas::ref(unsigned int x, unsigned int y)
	{
		return pixels.at(x + (y * width));
	}

	const Rgb& Canvas::ref(unsigned int x, unsigned int y) const
	{
		return pixels.at(x + (y * width));
	}

	void Canvas::loadBlackWhiteData(const std::vector<bool>& black_white_data)
	{
		unsigned int i = 0;
		for (const auto& px : black_white_data)
		{
			auto c = (uint8_t)(px * 255);
			pixels.at(i++) = Rgb{ c, c, c };
		}
	}

	void Canvas::addText(unsigned int x, unsigned int y, const std::string& text, const RasterFont& font)
	{
		return addText(x, y, unicode::utf8_to_utf32(text), font);
	}

	void Canvas::addText(unsigned int x, unsigned int y, const std::u32string& text, const RasterFont& font)
	{
		for (const auto& c : text)
		{
			const auto& g = font.get(c);
			addCanvas(x, y + g.y_offset, g.getCanvas());
			x += (g.width + 1);
		}
	}

	void Canvas::addCanvas(unsigned int x_offset, unsigned int y_offset, const Canvas& b)
	{
		for (unsigned int y = 0; y != b.height; ++y)
		{
			for (unsigned int x = 0; x != b.width; ++x)
			{
				set(x + x_offset, y + y_offset, b.get(x, y));
			}
		}
	}

	void Canvas::addRect(unsigned int x_offset, unsigned int y_offset, unsigned int width, unsigned int height, Rgb colour)
	{
		for (unsigned int y = 0; y != height; ++y)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				set(x + x_offset, y + y_offset, colour);
			}
		}
	}

	void Canvas::resize(unsigned int width, unsigned int height)
	{
		this->width = width;
		this->height = height;
		pixels.resize(width * height);
	}

	void Canvas::resizeWidth(unsigned int new_width)
	{
		std::vector<Rgb> new_pixels{};
		new_pixels.resize(new_width * height);
		for (unsigned int y = 0; y != height; ++y)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				new_pixels.at(x + (y * new_width)) = pixels.at(x + (y * width));
			}
		}
		width = new_width;
		pixels = std::move(new_pixels);
	}

	void Canvas::ensureWidthAndHeightAreEven()
	{
		auto even_width = ((width & 1) ? (width + 1) : width);
		if (width != even_width)
		{
			resizeWidth(even_width);
		}

		ensureHeightIsEven();
	}

	void Canvas::ensureHeightIsEven()
	{
		auto even_height = ((height & 1) ? (height + 1) : height);
		if (height != even_height)
		{
			pixels.resize(width * even_height);
			height = even_height;
		}
	}

	void Canvas::resizeNearestNeighbour(unsigned int desired_width, unsigned int desired_height)
	{
		Canvas c{ desired_width, desired_height };
		for (unsigned int y = 0; y != c.height; ++y)
		{
			for (unsigned int x = 0; x != c.width; ++x)
			{
				c.set(x, y, get(
					(unsigned int)(((double)x / c.width) * width),
					(unsigned int)(((double)y / c.height) * height)
				));
			}
		}
		*this = std::move(c);
	}

	void Canvas::resizeAveraged(unsigned int desired_width, unsigned int desired_height)
	{
		Canvas c{ desired_width, desired_height };
		const auto area_width = (unsigned int)((double)width / (double)desired_width);
		const auto area_height = (unsigned int)((double)height / (double)desired_height);
		for (unsigned int y = 0; y != c.height; ++y)
		{
			for (unsigned int x = 0; x != c.width; ++x)
			{
				c.set(x, y, getAverageOfArea(
					(unsigned int)(((double)x / c.width) * width),
					(unsigned int)(((double)y / c.height) * height),
					area_width,
					area_height
				));
			}
		}
		*this = std::move(c);
	}

	Rgb Canvas::getAverageOfArea(unsigned int _x, unsigned int _y, unsigned int width, unsigned int height) const
	{
		unsigned long long r = 0;
		unsigned long long g = 0;
		unsigned long long b = 0;
		for (int x = _x; x != _x + width; ++x)
		{
			for (int y = _y; y != _y + height; ++y)
			{
				Rgb colour = get(x, y);
				r += colour.r;
				g += colour.g;
				b += colour.b;
			}
		}
		r /= (width * height);
		g /= (width * height);
		b /= (width * height);
		return Rgb(r, g, b);
	}

	std::string Canvas::toString(bool explicit_nl) const
	{
		std::string str{};
		Rgb prev = pixels.front();
		++prev.r;
		for (unsigned int y = 0; y != height; ++y)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				const Rgb& colour = ref(x, y);
				if (colour != prev
					|| explicit_nl // bandaid
					)
				{
					prev = colour;
					str.append(console.strSetForegroundColour<std::string>(colour.r, colour.g, colour.b));
					str.append(console.strSetBackgroundColour<std::string>(colour.r, colour.g, colour.b));
				}
				str.push_back('-');
			}
			if (explicit_nl)
			{
				str.push_back('\n');
			}
		}
		return str;
	}

	std::string Canvas::toStringDoublewidth(bool explicit_nl) const
	{
		std::string str{};
		Rgb prev = pixels.front();
		++prev.r;
		for (unsigned int y = 0; y != height; ++y)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				const Rgb& colour = ref(x, y);
				if (colour != prev
					|| explicit_nl // bandaid
					)
				{
					prev = colour;
					str.append(console.strSetForegroundColour<std::string>(colour.r, colour.g, colour.b));
					str.append(console.strSetBackgroundColour<std::string>(colour.r, colour.g, colour.b));
				}
				str.append("--");
			}
			if (explicit_nl)
			{
				str.append(console.strResetColour<std::string>());
				str.push_back('\n');
			}
		}
		return str;
	}

	std::u16string Canvas::toStringDownsampled(bool explicit_nl, bool reset_on_nl)
	{
		ensureWidthAndHeightAreEven();

		std::u16string str{};
		str.reserve((unsigned int)width * height);
		Rgb bg = pixels.at(0);
		Rgb fg = bg;
		bool commited_bg = false;
		bool commited_fg = false;
		for (unsigned int y = 0; y != height; y += 2)
		{
			for (unsigned int x = 0; x != width; x += 2)
			{
				uint8_t chunkset = 0;
				{
					const Rgb& pxclr = pixels.at(x + (y * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b1000;
						}
						else
						{
							chunkset |= 0b1000;
							fg = pxclr;
							commited_fg = false;
						}
					}
				}
				{
					const Rgb& pxclr = pixels.at(x + 1 + (y * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b0100;
						}
						else if (commited_fg)
						{
							chunkset |= 0b0100;
							fg = pxclr;
							commited_fg = false;
						}
						else
						{
							bg = pxclr;
							commited_bg = false;
						}
					}
				}
				{
					const Rgb& pxclr = pixels.at(x + ((y + 1) * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b0010;
						}
						else if (commited_fg)
						{
							chunkset |= 0b00010;
							fg = pxclr;
							commited_fg = false;
						}
						else
						{
							bg = pxclr;
							commited_bg = false;
						}
					}
				}
				{
					const Rgb& pxclr = pixels.at(x + 1 + ((y + 1) * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b0001;
						}
						else if (commited_fg)
						{
							chunkset |= 0b00001;
							fg = pxclr;
							commited_fg = false;
						}
						else
						{
							bg = pxclr;
							commited_bg = false;
						}
					}
				}
				if (!commited_bg)
				{
					commited_bg = true;
					str.append(console.strSetBackgroundColour<std::u16string>(bg.r, bg.g, bg.b));
				}
				if (!commited_fg)
				{
					commited_fg = true;
					str.append(console.strSetForegroundColour<std::u16string>(fg.r, fg.g, fg.b));
				}
				str.push_back(downsampleChunkToChar(chunkset));
			}
			if (explicit_nl)
			{
				if (reset_on_nl)
				{
					commited_fg = false;
					commited_bg = false;
					str.append(console.strResetColour<std::u16string>());
				}
				str.push_back(u'\n');
			}
		}
		return str;
	}

	std::string Canvas::toStringDownsampledUtf8(bool explicit_nl, bool reset_on_nl)
	{
		return unicode::utf16_to_utf8(toStringDownsampled(explicit_nl, reset_on_nl));
	}

	std::u16string Canvas::toStringDownsampledDoublewidth(bool explicit_nl, bool reset_on_nl, std::optional<Rgb> filter)
	{
		ensureHeightIsEven();

		std::u16string str{};
		str.reserve((unsigned int)width * height);
		Rgb bg = filter.has_value() ? filter->invert() : pixels.at(0);
		Rgb fg = filter.has_value() ? *filter : bg;
		bool commited_bg = false;
		bool commited_fg = false;
		for (unsigned int y = 0; y != height; y += 2)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				uint8_t chunkset = 0;
				{
					const Rgb& pxclr = pixels.at(x + (y * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b1100;
						}
						else
						{
							chunkset |= 0b1100;
							if (!filter.has_value())
							{
								fg = pxclr;
							}
							commited_fg = false;
						}
					}
				}
				{
					const Rgb& pxclr = pixels.at(x + ((y + 1) * width));
					if (pxclr != bg)
					{
						if (pxclr == fg)
						{
							chunkset |= 0b0011;
						}
						else if (commited_fg)
						{
							chunkset |= 0b0011;
							if (!filter.has_value())
							{
								fg = pxclr;
							}
							commited_fg = false;
						}
						else
						{
							bg = pxclr;
							commited_bg = false;
						}
					}
				}
				if (!filter.has_value())
				{
					if (!commited_bg)
					{
						commited_bg = true;
						str.append(console.strSetBackgroundColour<std::u16string>(bg.r, bg.g, bg.b));
					}
					if (!commited_fg)
					{
						commited_fg = true;
						str.append(console.strSetForegroundColour<std::u16string>(fg.r, fg.g, fg.b));
					}
				}
				str.push_back(downsampleChunkToChar(chunkset));
			}
			if (explicit_nl)
			{
				if (reset_on_nl)
				{
					commited_fg = false;
					commited_bg = false;
					str.append(console.strResetColour<std::u16string>());
				}
				str.push_back(u'\n');
			}
		}
		return str;
	}

	std::string Canvas::toStringDownsampledDoublewidthUtf8(bool explicit_nl, bool reset_on_nl, std::optional<Rgb> filter)
	{
		return unicode::utf16_to_utf8(toStringDownsampledDoublewidth(explicit_nl, reset_on_nl, filter));
	}

	char16_t Canvas::downsampleChunkToChar(uint8_t chunkset) noexcept
	{
		switch (chunkset)
		{
			// 1 px
		case 0b1000: return u'\u2598'; // top left
		case 0b0100: return u'\u259D'; // top right
		case 0b0010: return u'\u2596'; // bottom left
		case 0b0001: return u'\u2597'; // bottom right
			// 2 px, sides
		case 0b1100: return u'\u2580';
		case 0b0011: return u'\u2584';
		case 0b1010: return u'\u258C';
		case 0b0101: return u'\u2590';
			// 2 px, corners
		case 0b1001: return u'\u259A';
		case 0b0110: return u'\u259E';
			// 3 px
		case 0b0111: return u'\u259F';
		case 0b1011: return u'\u2599';
		case 0b1101: return u'\u259C';
		case 0b1110: return u'\u259B';
			// 4 px
		case 0b1111: return u'\u2588';
		}
		// 0 px
		return ' ';
	}

	Canvas Canvas::fromBmp(Reader& r)
	{
		Canvas c;

		uint16_t sig;
		SOUP_IF_LIKELY (r.u16le(sig) && sig == 0x4D42)
		{
			uint32_t data_start, header_size, palette_size;
			int32_t width, height;
			int16_t bits_per_pixel;
			SOUP_IF_LIKELY ((r.seek(0x0A), r.u32le(data_start))
				&& r.u32le(header_size)
				&& header_size == 40 // BITMAPINFOHEADER
				&& r.i32le(width)
				&& r.i32le(height)
				&& r.skip(2) // planes
				&& r.i16le(bits_per_pixel)
				&& r.skip(4) // compression method
				&& r.skip(4) // image size
				&& r.skip(4) // horizontal resolution
				&& r.skip(4) // vertical resolution
				&& r.u32le(palette_size)
				&& r.skip(4) // important colours
				)
			{
				const bool y_inverted = (height >= 0);
				if (!y_inverted)
				{
					height *= -1;
				}
				c.resize(width, height);

				if (bits_per_pixel == 4)
				{
					Rgb palette[/* 1 << 4 */ 0x10];

					for (uint32_t i = 0; i != palette_size; ++i)
					{
						SOUP_IF_UNLIKELY (!r.u8(palette[i].b)
							|| !r.u8(palette[i].g)
							|| !r.u8(palette[i].r)
							|| !r.skip(1)
							)
						{
							break;
						}
						//console.setForegroundColour(palette[i]);
						//console << "Palette colour " << i << "\n";
					}

					r.seek(data_start);
					int32_t pixels_remaining_on_this_line = width;
					for (size_t i = 0; i != c.pixels.size(); )
					{
						uint32_t dw;
						SOUP_IF_UNLIKELY (!r.u32le(dw))
						{
							break;
						}

						for (uint8_t j = 0; j != 8; ++j)
						{
							c.pixels[i++] = palette[(dw >> ((j ^ 1) * 4)) & 0xf];
							if (--pixels_remaining_on_this_line == 0)
							{
								pixels_remaining_on_this_line = width;
								break;
							}
						}
					}
				}
				else
				{
					r.seek(data_start);
					const auto bytes_per_line = ((width * bits_per_pixel + 31) / 32) * 4;
					const auto pad_bytes = (4 - (bytes_per_line % 4)) % 4;
					int32_t pixels_remaining_on_this_line = width;
					for (auto& p : c.pixels)
					{
						SOUP_IF_UNLIKELY (!r.u8(p.b)
							|| !r.u8(p.g)
							|| !r.u8(p.r)
							)
						{
							break;
						}
						if (--pixels_remaining_on_this_line == 0)
						{
							r.skip(pad_bytes);
							pixels_remaining_on_this_line = width;
						}
					}
				}

				if (y_inverted)
				{
					for (int y = 0; y != height / 2; ++y)
					{
						for (int x = 0; x != width; ++x)
						{
							std::swap(c.ref(x, y), c.ref(x, height - 1 - y));
						}
					}
				}
			}
		}

		return c;
	}

	std::string Canvas::toSvg(unsigned int scale) const
	{
		std::string str = R"(<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width=")";
		str.append(std::to_string(width * scale));
		str.append(R"(" height=")");
		str.append(std::to_string(height * scale));
		str.append(R"(">)");

		std::string rect_suffix = R"(" width=")";
		rect_suffix.append(std::to_string(scale));
		rect_suffix.append(R"(" height=")");
		rect_suffix.append(std::to_string(scale));
		rect_suffix.append("\"/>");
		for (unsigned int y = 0; y != height; ++y)
		{
			for (unsigned int x = 0; x != width; ++x)
			{
				str.append(R"(<rect x=")");
				str.append(std::to_string(x * scale));
				str.append(R"(" y=")");
				str.append(std::to_string(y * scale));
				str.append(R"(" fill="#)");
				str.append(get(x, y).toHex());
				str.append(rect_suffix);
			}
		}
		str.append("</svg>");
		return str;
	}

	std::string Canvas::toPng() const
	{
		StringWriter sw;
		toPng(sw);
		return sw.data;
	}

	void Canvas::toPng(Writer& w) const
	{
		TinyPngOut po(width, height, w);
		po.write(pixels.data(), pixels.size());
	}

	std::string Canvas::toPpm() const
	{
		std::string res = "P3\n";
		res.append(std::to_string(width));
		res.push_back(' ');
		res.append(std::to_string(height));
		res.append("\n255\n");
		for (const auto& p : pixels)
		{
			res.append(std::to_string(p.r)).push_back(' ');
			res.append(std::to_string(p.g)).push_back(' ');
			res.append(std::to_string(p.b)).push_back('\n');
		}
		return res;
	}

	std::string Canvas::toBmp() const
	{
		StringWriter sw;
		toBmp(sw);
		return sw.data;
	}

	bool Canvas::toBmp(Writer& w) const
	{
		uint16_t s;
		uint32_t i;
		SOUP_IF_UNLIKELY (!(s = 0x4D42, w.u16le(s))
			|| !(i = static_cast<uint32_t>(40 + (3 * pixels.size())), w.u32le(i))
			|| !w.skip(4)
			|| !(i = (14 + 40), w.u32le(i))
			|| !(i = 40, w.u32le(i))
			|| !(i = width, w.u32le(i))
			|| !(i = height * -1, w.u32le(i))
			|| !(s = 1, w.u16le(s))
			|| !(s = 24, w.u16le(s))
			|| !(i = 0, w.u32le(i))
			|| !(i = static_cast<uint32_t>(3 * pixels.size()), w.u32le(i))
			|| !w.skip(4 + 4 + 4 + 4)
			)
		{
			return false;
		}
		for (const auto& p : pixels)
		{
			SOUP_IF_UNLIKELY (!w.u8(const_cast<uint8_t&>(p.r))
				|| !w.u8(const_cast<uint8_t&>(p.g))
				|| !w.u8(const_cast<uint8_t&>(p.b))
				)
			{
				break;
			}
		}
		return true;
	}
}
