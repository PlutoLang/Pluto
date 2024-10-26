#pragma once

#include <string>
#include <vector>

#include "base.hpp"
#include "fwd.hpp"

#include "Rgb.hpp"

NAMESPACE_SOUP
{
	class Canvas
	{
	public:
		unsigned int width = 0;
		unsigned int height = 0;
		std::vector<Rgb> pixels{};

		Canvas() noexcept = default;

		Canvas(unsigned int size)
			: Canvas(size, size)
		{
		}

		Canvas(unsigned int width, unsigned int height)
			: width(width), height(height)
		{
			pixels.resize(width * height);
		}

		Canvas(unsigned int width, unsigned int height, const std::vector<bool>& black_white_data)
			: Canvas(width, height)
		{
			loadBlackWhiteData(black_white_data);
		}

		void fill(const Rgb colour);
		void set(unsigned int x, unsigned int y, Rgb colour) noexcept;
		[[nodiscard]] Rgb get(unsigned int x, unsigned int y) const;
		[[nodiscard]] Rgb& ref(unsigned int x, unsigned int y);
		[[nodiscard]] const Rgb& ref(unsigned int x, unsigned int y) const;

		void loadBlackWhiteData(const std::vector<bool>& black_white_data);

		void addText(unsigned int x, unsigned int y, const std::string& text, const RasterFont& font);
		void addText(unsigned int x, unsigned int y, const std::u32string& text, const RasterFont& font);
		void addCanvas(unsigned int x_offset, unsigned int y_offset, const Canvas& b);
		void addRect(unsigned int x_offset, unsigned int y_offset, unsigned int width, unsigned int height, Rgb colour);

		void resize(unsigned int width, unsigned int height); // Fine for change in height; either trims excess or inserts black pixels below.
		void resizeWidth(unsigned int new_width); // Resize algorithm for width; either trims excess or inserts black pixels to the right.
		void ensureWidthAndHeightAreEven();
		void ensureHeightIsEven();

		void resizeNearestNeighbour(unsigned int desired_width, unsigned int desired_height); // Resizes the canvas and its contents, works for all changes.
		void resizeAveraged(unsigned int desired_width, unsigned int desired_height); // Resizes the canvas and its contents, works for all changes.

		[[nodiscard]] Rgb getAverageOfArea(unsigned int x, unsigned int y, unsigned int width, unsigned int height) const;

		[[nodiscard]] std::string toString(bool explicit_nl) const;
		[[nodiscard]] std::string toStringDoublewidth(bool explicit_nl) const;
		[[nodiscard]] std::u16string toStringDownsampled(bool explicit_nl, bool reset_on_nl);
		[[nodiscard]] std::string toStringDownsampledUtf8(bool explicit_nl, bool reset_on_nl);
		[[nodiscard]] std::u16string toStringDownsampledDoublewidth(bool explicit_nl, bool reset_on_nl, std::optional<Rgb> filter = std::nullopt);
		[[nodiscard]] std::string toStringDownsampledDoublewidthUtf8(bool explicit_nl, bool reset_on_nl, std::optional<Rgb> filter = std::nullopt);
	private:
		[[nodiscard]] static char16_t downsampleChunkToChar(uint8_t chunkset) noexcept;

	public:
		[[nodiscard]] static Canvas fromBmp(Reader& r);

		[[nodiscard]] std::string toSvg(unsigned int scale = 1) const;
		[[nodiscard]] std::string toPng() const;
		void toPng(Writer& w) const;
		[[nodiscard]] std::string toPpm() const; // Bit of a niche format, but dead simple to write. You can load images of this type with GIMP.
		[[nodiscard]] std::string toBmp() const;
		bool toBmp(Writer& w) const;
	};
}
