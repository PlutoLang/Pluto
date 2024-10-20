#include "TinyPngOut.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

#include "adler32.hpp"
#include "crc32.hpp"
#include "Endian.hpp"

NAMESPACE_SOUP
{
	TinyPngOut::TinyPngOut(uint32_t w, uint32_t h, Writer& out)
		: width(w),
		height(h),
		output(out),
		positionX(0),
		positionY(0),
		deflateFilled(0),
		adler(1)
	{
		SOUP_ASSERT(width != 0 && height != 0); // Bad resolution?

		// Compute and check data siezs
		uint64_t lineSz = static_cast<uint64_t>(width) * 3 + 1;
		SOUP_ASSERT(lineSz <= UINT32_MAX); // Image too large?
		lineSize = static_cast<uint32_t>(lineSz);

		uint64_t uncompRm = lineSize * height;
		SOUP_ASSERT(uncompRm <= UINT32_MAX); // Image too large?
		uncompRemain = static_cast<uint32_t>(uncompRm);

		uint32_t numBlocks = uncompRemain / DEFLATE_MAX_BLOCK_SIZE;
		if (uncompRemain % DEFLATE_MAX_BLOCK_SIZE != 0)
		{
			numBlocks++;  // Round up
		}
		// 5 bytes per DEFLATE uncompressed block header, 2 bytes for zlib header, 4 bytes for zlib Adler-32 footer
		uint64_t idatSize = static_cast<uint64_t>(numBlocks) * 5 + 6;
		idatSize += uncompRemain;
		SOUP_ASSERT(idatSize <= static_cast<uint32_t>(INT32_MAX)); // Image too large?

		// Write header (not a pure header, but a couple of things concatenated together)
		uint8_t header[] = {  // 43 bytes long
			// PNG header
			0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
			// IHDR chunk
			0x00, 0x00, 0x00, 0x0D,
			0x49, 0x48, 0x44, 0x52,
			0, 0, 0, 0,  // 'width' placeholder
			0, 0, 0, 0,  // 'height' placeholder
			0x08, 0x02, 0x00, 0x00, 0x00,
			0, 0, 0, 0,  // IHDR CRC-32 placeholder
			// IDAT chunk
			0, 0, 0, 0,  // 'idatSize' placeholder
			0x49, 0x44, 0x41, 0x54,
			// DEFLATE data
			0x08, 0x1D,
		};
		putBigUint32(width, &header[16]);
		putBigUint32(height, &header[20]);
		putBigUint32(static_cast<uint32_t>(idatSize), &header[33]);
		crc = 0;
		updateCrc(&header[12], 17);
		putBigUint32(crc, &header[29]);
		write(header);

		crc = 0;
		updateCrc(&header[37], 6);  // 0xD7245B6B
	}

	void TinyPngOut::write(const Rgb* pixels, size_t count)
	{
		write(reinterpret_cast<const uint8_t*>(pixels), count);
	}

	void TinyPngOut::write(const uint8_t* pixels, size_t count)
	{
		SOUP_ASSERT(count <= SIZE_MAX / 3);
		count *= 3;  // Convert pixel count to byte count
		while (count > 0)
		{
			SOUP_ASSERT(positionY < height); // All image pixels already written?

			if (deflateFilled == 0) // Start DEFLATE block
			{
				uint16_t size = DEFLATE_MAX_BLOCK_SIZE;
				if (uncompRemain < size)
					size = static_cast<uint16_t>(uncompRemain);
				const uint8_t header[] = {  // 5 bytes long
					static_cast<uint8_t>(uncompRemain <= DEFLATE_MAX_BLOCK_SIZE ? 1 : 0),
					static_cast<uint8_t>(size >> 0),
					static_cast<uint8_t>(size >> 8),
					static_cast<uint8_t>((size >> 0) ^ 0xFF),
					static_cast<uint8_t>((size >> 8) ^ 0xFF),
				};
				write(header);
				updateCrc(header, sizeof(header) / sizeof(header[0]));
			}
			assert(positionX < lineSize && deflateFilled < DEFLATE_MAX_BLOCK_SIZE);

			if (positionX == 0)
			{
				// Beginning of line - write filter method byte
				uint8_t b[] = { 0 };
				write(b);
				updateCrc(b, 1);
				updateAdler(b, 1);
				positionX++;
				uncompRemain--;
				deflateFilled++;
			}
			else
			{
				// Write some pixel bytes for current line
				uint16_t n = DEFLATE_MAX_BLOCK_SIZE - deflateFilled;
				if (lineSize - positionX < n)
				{
					n = static_cast<uint16_t>(lineSize - positionX);
				}
				if (count < n)
				{
					n = static_cast<uint16_t>(count);
				}
				if (static_cast<std::make_unsigned<size_t>::type>(std::numeric_limits<size_t>::max()) < std::numeric_limits<decltype(n)>::max())
				{
					n = std::min(n, static_cast<decltype(n)>(std::numeric_limits<size_t>::max()));
				}
				assert(n > 0);
				output.raw(const_cast<uint8_t*>(pixels), static_cast<size_t>(n));

				// Update checksums
				updateCrc(pixels, n);
				updateAdler(pixels, n);

				// Increment positions
				count -= n;
				pixels += n;
				positionX += n;
				uncompRemain -= n;
				deflateFilled += n;
			}

			if (deflateFilled >= DEFLATE_MAX_BLOCK_SIZE)
			{
				deflateFilled = 0;  // End current block
			}

			if (positionX == lineSize) // Increment line
			{
				positionX = 0;
				positionY++;
				if (positionY == height) // Reached end of pixels
				{
					uint8_t footer[] = {  // 20 bytes long
						0, 0, 0, 0,  // DEFLATE Adler-32 placeholder
						0, 0, 0, 0,  // IDAT CRC-32 placeholder
						// IEND chunk
						0x00, 0x00, 0x00, 0x00,
						0x49, 0x45, 0x4E, 0x44,
						0xAE, 0x42, 0x60, 0x82,
					};
					putBigUint32(adler, &footer[0]);
					updateCrc(&footer[0], 4);
					putBigUint32(crc, &footer[4]);
					write(footer);
				}
			}
		}
	}

	void TinyPngOut::updateCrc(const uint8_t* data, size_t size)
	{
		crc = crc32::hash(data, size, crc);
	}

	void TinyPngOut::updateAdler(const uint8_t* data, size_t size)
	{
		adler = adler32::hash(data, size, adler);
	}

	void TinyPngOut::putBigUint32(uint32_t val, uint8_t arr[4])
	{
		if constexpr (ENDIAN_NATIVE == ENDIAN_LITTLE)
		{
			*reinterpret_cast<uint32_t*>(&arr[0]) = Endianness::invert(val);
		}
		else
		{
			*reinterpret_cast<uint32_t*>(&arr[0]) = val;
		}

		/*for (int i = 0; i != 4; ++i)
		{
			arr[i] = static_cast<uint8_t>(val >> ((3 - i) * 8));
		}*/
	}
}
