
// WORK IN PROGRESS
// LOCALLY IMPLEMENTED
// NEED TO CONFIGURE IT PROPERLY
// CHECK IF VALID SYNTAX
// JUST THE IDEA I AM GOING AFTER (from security lessons)




// #include "security.h"
// #include <wolfssl/wolfcrypt/aes.h>
// #include <wolfssl/ssl.h>

// static byte key[AES_128_KEY_SIZE];
// static byte iv[AES_IV_SIZE];
// static WC_RNG rng;

// void initialize_crypto(void) {
//     wc_InitRng(&rng);
//     wc_RNG_GenerateBlock(&rng, key, sizeof(key));
//     wc_RNG_GenerateBlock(&rng, iv, sizeof(iv));
// }

// int encrypt_data(const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext, size_t *ciphertext_len) {
//     Aes enc_ctx;
//     int ret;

//     wc_AesInit(&enc_ctx, NULL, INVALID_DEVID);
//     ret = wc_AesSetKey(&enc_ctx, key, sizeof(key), iv, AES_ENCRYPTION);
//     if (ret != 0) return ret;

//     ret = wc_AesCbcEncrypt(&enc_ctx, ciphertext, plaintext, plaintext_len);
//     *ciphertext_len = plaintext_len;  // CBC keeps size the same
//     wc_AesFree(&enc_ctx);

//     return ret;
// }

// int decrypt_data(const uint8_t *ciphertext, size_t ciphertext_len, uint8_t *plaintext, size_t *plaintext_len) {
//     Aes dec_ctx;
//     int ret;

//     wc_AesInit(&dec_ctx, NULL, INVALID_DEVID);
//     ret = wc_AesSetKey(&dec_ctx, key, sizeof(key), iv, AES_DECRYPTION);
//     if (ret != 0) return ret;

//     ret = wc_AesCbcDecrypt(&dec_ctx, plaintext, ciphertext, ciphertext_len);
//     *plaintext_len = ciphertext_len;  // assume no padding for initial semplicity
//     wc_AesFree(&dec_ctx);

//     return ret;
// }
