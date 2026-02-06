#include "CryptoHashAlgo.hpp"
#include <string>

NAMESPACE_SOUP
{
	struct whirlpool : CryptoHashAlgo<whirlpool>
	{
		static constexpr auto BLOCK_BYTES = 64u;
		static constexpr auto DIGEST_BYTES = 64u;

		[[nodiscard]] static std::string hash(const void *data, size_t len);
		[[nodiscard]] static std::string hash(const std::string &str)
		{
			return hash(str.data(), str.size());
		}

		struct State
		{
			uint64_t state[8];
			uint8_t buffer[BLOCK_BYTES];
			uint64_t n_bytes;

			State() noexcept;

			void appendByte(uint8_t byte) noexcept
			{
				append(&byte, 1);
			}

			void append(const void* data, size_t size)
			{
				unsigned index = (unsigned)this->n_bytes & 63;
				unsigned left;
				this->n_bytes += size;

				if (index)
				{
					left = BLOCK_BYTES - index;
					memcpy(this->buffer + index, data,
						   (size < left ? size : left));
					if (size < left)
						return;

					processBlock((uint64_t *)this->buffer);
					data = (uint8_t*)data + left;
					size -= left;
				}
				while (size >= BLOCK_BYTES)
				{
					uint64_t* aligned_message_block;
					if (((uintptr_t)(data) & 7) == 0)
					{
						aligned_message_block = (uint64_t*)data;
					}
					else
					{
						memcpy(this->buffer, data, BLOCK_BYTES);
						aligned_message_block = (uint64_t *)this->buffer;
					}

					processBlock(aligned_message_block);
					data = (uint8_t*)data + BLOCK_BYTES;
					size -= BLOCK_BYTES;
				}
				if (size)
				{
					memcpy(this->buffer, data, size);
				}
			}

			void finalise();

			void processBlock(uint64_t *p_block);

			void getDigest(uint8_t out[DIGEST_BYTES]) const noexcept;
			[[nodiscard]] std::string getDigest() const;
		};
	};
}
