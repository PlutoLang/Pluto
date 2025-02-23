#pragma once

#include <cstdint>
#include <string>

#include "base.hpp" // SOUP_EXCAL

NAMESPACE_SOUP
{
	struct aes
	{
		// Input size should be a multiple of 16 bytes.
		//   GCM can deal with unaligned data, other methods will simply ignore the trailing bytes -> they will not be encrypted.
		//   You may use a padding scheme such as PKCS#7 for padding.
		// Key size must be 16 bytes, 24 bytes, or 32 bytes.

		static void pkcs7Pad(std::string& encrypted) noexcept;
		[[nodiscard]] static bool pkcs7Unpad(std::string& decrypted) noexcept;

		static void cbcEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]) noexcept;
		static void cbcDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]) noexcept;
		static void cfbEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]) noexcept;
		static void cfbDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]) noexcept;
		static void ecbEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len) noexcept;
		static void ecbDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len) noexcept;
		static void gcmEncrypt(uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len, uint8_t tag[16]) noexcept;
		static bool gcmDecrypt(uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len, const uint8_t tag[16]) noexcept;


		static void expandKey(uint8_t w[240], const uint8_t* key, size_t key_len) noexcept;
		static void expandKeyForDecryption(uint8_t w[240], const uint8_t* key, size_t key_len) noexcept;
		[[nodiscard]] static int getNk(size_t key_len) noexcept;
		[[nodiscard]] static int getNrFromKeyLen(size_t key_len) noexcept;
		[[nodiscard]] static int getNrFromNk(const int Nk) noexcept;

		static void subBytes(uint8_t** state) noexcept;
		static void shiftRow(uint8_t** state, int i, int n) noexcept;    // shift row i on n positions
		static void shiftRows(uint8_t** state) noexcept;
		static uint8_t xtime(uint8_t b) noexcept;    // multiply on x
		static void mixColumns(uint8_t** state) noexcept;
		static void addRoundKey(uint8_t** state, const uint8_t* key) noexcept;
		static void subWord(uint8_t* a) noexcept;
		static void rotWord(uint8_t* a) noexcept;
		static void xorWords(uint8_t* a, uint8_t* b, uint8_t* c) noexcept;
		[[nodiscard]] static uint8_t getRoundConstant(int n) noexcept;
		static void invSubBytes(uint8_t** state) noexcept;
		static void invMixColumns(uint8_t** state) noexcept;
		static void invShiftRows(uint8_t** state) noexcept;
		static void encryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240], const int Nr) noexcept;
		static void decryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240], const int Nr) noexcept;
		static SOUP_FORCEINLINE void xorBlocks(uint8_t a[16], const uint8_t b[16]) noexcept;
		static void xorBlocks(uint8_t a[], const uint8_t b[], unsigned int len) noexcept;

		static SOUP_FORCEINLINE void rshiftBlock(uint8_t block[16]) noexcept;
		static void mulBlocks(uint8_t res[16], const uint8_t x[16], const uint8_t y[16]) noexcept;
		static void calcH(uint8_t h[16], uint8_t roundKeys[240], const int Nr) noexcept;
		static void calcJ0(uint8_t j0[16], const uint8_t h[16], const uint8_t* iv, size_t iv_len) noexcept;
		static void inc32(uint8_t block[16]) noexcept;
		static void gctr(uint8_t* data, size_t data_len, const uint8_t roundKeys[240], const int Nr, const uint8_t icb[8]) noexcept;
		static void calcGcmTag(uint8_t tag[16], uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t roundKeys[16], const int Nr, const uint8_t h[16], const uint8_t j0[16]) noexcept;

		struct GhashState
		{
			uint8_t* const res;
			const uint8_t* const h;
			uint8_t buffer[16];
			uint8_t buffer_counter;

			GhashState(uint8_t res[16], const uint8_t h[16]) noexcept;

			void append(const uint8_t data[], size_t size) noexcept
			{
				for (size_t i = 0; i != size; ++i)
				{
					appendByte(data[i]);
				}
			}
			
			void appendByte(uint8_t byte) noexcept
			{
				buffer[buffer_counter] = byte;
				if (++buffer_counter == 16)
				{
					buffer_counter = 0;
					transform();
				}
			}

			void transform() noexcept;
		};
	};
}
