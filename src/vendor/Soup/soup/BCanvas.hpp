#pragma once

#include <vector>

#include "fwd.hpp"

NAMESPACE_SOUP
{
	// Black & White Canvas.
	struct BCanvas
	{
		unsigned int width = 0;
		unsigned int height = 0;
		std::vector<bool> pixels{};

		BCanvas() = default;

		BCanvas(unsigned int size)
			: BCanvas(size, size)
		{
		}

		BCanvas(unsigned int width, unsigned int height)
			: width(width), height(height)
		{
			pixels.resize(width * height);
		}

		BCanvas(unsigned int width, unsigned int height, std::vector<bool> pixels)
			: width(width), height(height), pixels(std::move(pixels))
		{
		}

		void set(unsigned int x, unsigned int y, bool b = true) noexcept;
		[[nodiscard]] bool get(unsigned int x, unsigned int y) const;

		void addLine(const Vector2& a, const Vector2& b);
		void addQuadraticBezier(const Vector2& a, const Vector2& b, const Vector2& c);
		void addCubicBezier(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& d);

		void floodFill(unsigned int x, unsigned int y);

		[[nodiscard]] Canvas toCanvas() const;
	};
}
