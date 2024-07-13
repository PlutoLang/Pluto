#pragma once

#include "fwd.hpp"

#include <string>
#include <utility>
#include <vector>

#include "Rgb.hpp"

NAMESPACE_SOUP
{
	struct FormattedText
	{
		struct Colour
		{
			bool reset = false;
			Rgb rgb;

			Colour(Rgb rgb)
				: reset(false), rgb(rgb)
			{
			}

			Colour(bool reset)
				: reset(reset)
			{
			}
		};

		struct Span
		{
			std::string text;
			Colour fg;
			Colour bg;

			Span(std::string text)
				: text(std::move(text)), fg(true), bg(true)
			{
			}

			Span(std::string text, Rgb fg)
				: text(std::move(text)), fg(fg), bg(true)
			{
			}

			Span(std::string text, Rgb fg, Rgb bg)
				: text(std::move(text)), fg(fg), bg(bg)
			{
			}
		};

		std::vector<std::vector<Span>> lines{};

		void addSpan(std::string text);
		void addSpan(std::string text, Rgb fg);
		void addSpan(std::string text, Rgb fg, Rgb bg);
		void addSpan(Span&& span);
		void newLine();

		[[nodiscard]] std::pair<size_t, size_t> measure(const RasterFont& font) const;

		void draw(RenderTarget& rt, const RasterFont& font) const;

		[[nodiscard]] std::string toString() const; // uses ANSI escape codes
		[[nodiscard]] std::string toHtml() const;
	};
}
