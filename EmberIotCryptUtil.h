#ifndef FIREPROP_CRYPTUTIL_H
#define FIREPROP_CRYPTUTIL_H

extern "C" {
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/base64.h>
}

#include <Arduino.h>

inline int base64urlencode(char *arr)
{
    size_t length = strlen(arr);
    for (size_t i = 0; i < length; i++)
    {
        if (arr[i] == '+')
        {
            arr[i] = '-';
        }
        else if (arr[i] == '/')
        {
            arr[i] = '_';
        }

        if (arr[i] == '=')
        {
            return length - i;
        }
    }

    return 0;
}

inline int base64encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
    return mbedtls_base64_encode((unsigned char*) dst, dlen, olen, src, slen);
}

inline void sha256Hash(const char *input, byte hash[32]) {
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);
    mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&md_ctx);
    mbedtls_md_update(&md_ctx, (const unsigned char*) input, strlen(input));
    mbedtls_md_finish(&md_ctx, hash);
    mbedtls_md_free(&md_ctx);
}

inline int signRS256(const char *message, const char *privateKey, char *output, size_t outputSize) {
    // Prepare mbedTLS contexts
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random generator (entropy)
    const char *pers = "esp32_jwt";
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers, strlen(pers));

    // Parse the RSA private key (PEM)
    int ret = mbedtls_pk_parse_key(&pk, (const unsigned char*) privateKey,strlen(privateKey)+1, nullptr, 0);
    if (ret != 0) {
        Serial.printf("Failed to parse private key: %d\n", ret);
        return ret;
    }

    // Compute SHA-256 hash of the message
    uint8_t hash[32];
    mbedtls_sha256((const unsigned char*) message, strlen(message), hash, 0);

    // Sign the hash
    size_t sig_len = 0;
    uint8_t sig[512]; // enough for RSA-2048
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32,sig, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        Serial.printf("Failed to sign JWT: %d\n", ret);
        return ret;
    }

    // Clean up mbedTLS contexts
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    // Base64url-encode the signature
    size_t written = 0;
    base64encode((unsigned char*) output, outputSize, &written, sig, sig_len);
    return 0;
}

#endif