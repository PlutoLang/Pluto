#pragma once

#include <cstddef>
#include <cstdint>

#include "fwd.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	/*
	 * Original source: https://www.nayuki.io/page/tiny-png-output
	 * Original licence follows.
	 *
	 * Copyright (c) 2018 Project Nayuki
	 *
	 * This program is free software: you can redistribute it and/or modify
	 * it under the terms of the GNU Lesser General Public License as published by
	 * the Free Software Foundation, either version 3 of the License, or
	 * (at your option) any later version.
	 *
	 * This program is distributed in the hope that it will be useful,
	 * but WITHOUT ANY WARRANTY; without even the implied warranty of
	 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 * GNU Lesser General Public License for more details.
	 *
	 * You should have received a copy of the GNU Lesser General Public License
	 * along with this program (see COPYING.txt and COPYING.LESSER.txt).
	 * If not, see <http://www.gnu.org/licenses/>.
	 */

	/*
	 * Takes image pixel data in raw RGB8.8.8 format and writes a PNG file to a byte output stream.
	 */
	class TinyPngOut
	{
	private:
		const uint32_t width;
		const uint32_t height;
		uint32_t lineSize;  // Measured in bytes, equal to (width * 3 + 1)
		Writer& output;
		uint32_t positionX;      // Next byte index in current line
		uint32_t positionY;      // Line index of next byte
		uint32_t uncompRemain;   // Number of uncompressed bytes remaining
		uint16_t deflateFilled;  // Bytes filled in the current block (0 <= n < DEFLATE_MAX_BLOCK_SIZE)
		uint32_t crc;    // Primarily for IDAT chunk
		uint32_t adler;  // For DEFLATE data within IDAT

	public:
		/*
		 * Creates a PNG writer with the given width and height (both non-zero) and byte output stream.
		 * TinyPngOut will leave the output stream still open once it finishes writing the PNG file data.
		 * Throws an exception if the dimensions exceed certain limits (e.g. w * h > 700 million).
		 */
		explicit TinyPngOut(uint32_t w, uint32_t h, Writer& out);
		TinyPngOut(const TinyPngOut& other) = delete;
		TinyPngOut(TinyPngOut&& other) = default;

		void write(const Rgb* pixels, size_t count);
		void write(const uint8_t* pixels, size_t count);

	private:
		void updateCrc(const uint8_t* data, size_t size);
		void updateAdler(const uint8_t* data, size_t size);

		template <size_t N>
		void write(const uint8_t(&data)[N])
		{
			output.raw(const_cast<char*>(reinterpret_cast<const char*>(data)), sizeof(data));
		}

		static void putBigUint32(uint32_t val, uint8_t arr[4]);

		static constexpr uint16_t DEFLATE_MAX_BLOCK_SIZE = UINT16_C(65535);
	};
}
