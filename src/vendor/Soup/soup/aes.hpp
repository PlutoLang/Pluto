#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace soup
{
	class aes
	{
	public:
		// Input size should be a multiple of 16 bytes.
		//   GCM can deal with unaligned data, other methods will simply ignore the trailing bytes -> they will not be encrypted.
		//   You may use a padding scheme such as PKCS#7 for padding.
		// Key size must be 16 bytes, 24 bytes, or 32 bytes.
		// IV size must be 16 bytes.

		static void pkcs7Pad(std::string& encrypted) noexcept;
		[[nodiscard]] static bool pkcs7Unpad(std::string& decrypted) noexcept;

		static void cbcEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]);
		static void cbcDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]);
		static void cfbEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]);
		static void cfbDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len, const uint8_t iv[16]);
		static void ecbEncrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len);
		static void ecbDecrypt(uint8_t* data, size_t data_len, const uint8_t* key, size_t key_len);
		static void gcmEncrypt(uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len, uint8_t tag[16]);
		static bool gcmDecrypt(uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len, const uint8_t tag[16]);

	private:
		static void expandKey(uint8_t w[240], const uint8_t* key, size_t key_len);
		[[nodiscard]] static int getNk(size_t key_len);
		[[nodiscard]] static int getNr(size_t key_len);
		[[nodiscard]] static int getNr(const int Nk);

		static void subBytes(uint8_t** state);
		static void shiftRow(uint8_t** state, int i, int n);    // shift row i on n positions
		static void shiftRows(uint8_t** state);
		static uint8_t xtime(uint8_t b);    // multiply on x
		static void mixColumns(uint8_t** state);
		static void addRoundKey(uint8_t** state, const uint8_t* key);
		static void subWord(uint8_t* a);
		static void rotWord(uint8_t* a);
		static void xorWords(uint8_t* a, uint8_t* b, uint8_t* c);
		[[nodiscard]] static uint8_t getRoundConstant(int n);
		static void invSubBytes(uint8_t** state);
		static void invMixColumns(uint8_t** state);
		static void invShiftRows(uint8_t** state);
		static void encryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240], const int Nr);
		static void decryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240], const int Nr);
		static void xorBlocks(uint8_t a[16], const uint8_t b[16]);
		static void xorBlocks(uint8_t a[16], const uint8_t b[16], unsigned int len);

		static void ghash(uint8_t res[16], const uint8_t h[16], const std::vector<uint8_t>& x);
		static void calcH(uint8_t h[16], uint8_t roundKeys[240], const int Nr);
		static void calcJ0(uint8_t j0[16], const uint8_t h[16], const uint8_t* iv, size_t iv_len);
		static void inc32(uint8_t block[16]);
		static void gctr(uint8_t* data, size_t data_len, const uint8_t roundKeys[240], const int Nr, const uint8_t icb[8]);
		static void calcGcmTag(uint8_t tag[16], uint8_t* data, size_t data_len, const uint8_t* aadata, size_t aadata_len, const uint8_t roundKeys[16], const int Nr, const uint8_t h[16], const uint8_t j0[16]);
	};
}
