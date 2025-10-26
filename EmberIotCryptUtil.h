/******************************************************************************
* Project Name: EmberIoT
*
* Ember IoT is a simple proof of concept for a Firebase-hosted IoT
* cloud designed to work with Arduino-based devices and an Android mobile app.
* It enables microcontrollers to connect to the cloud, sync data,
* and interact with a mobile interface using Firebase Authentication and
* Firebase Realtime Database services. This project simplifies creating IoT
* infrastructure without the need for a dedicated server.
*
* Copyright (c) 2025 davirxavier
*
* MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*****************************************************************************/

#ifndef FIREPROP_CRYPTUTIL_H
#define FIREPROP_CRYPTUTIL_H

#include <Arduino.h>

#ifdef ESP32
extern "C" {
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/base64.h>
}
#elif ESP8266
#include <BearSSLHelpers.h>
#include <Ember_BearSSL_RSA.h>
#endif

#ifdef ESP8266
static const char B64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline size_t base64_encode(const uint8_t *data, size_t len, unsigned char *out, size_t out_size, size_t *written) {
    // Each 3 bytes → 4 chars, plus padding. Total length = 4 * ceil(len/3)
    size_t needed = 4 * ((len + 2) / 3);
    if (out_size < needed + 1) return -1;  // +1 for null terminator

    size_t i = 0, o = 0;
    // Full 3‑byte chunks
    for (; i + 2 < len; i += 3) {
        uint32_t v = (data[i] << 16) | (data[i+1] << 8) | data[i+2];
        out[o++] = B64_TABLE[(v >> 18) & 0x3F];
        out[o++] = B64_TABLE[(v >> 12) & 0x3F];
        out[o++] = B64_TABLE[(v >>  6) & 0x3F];
        out[o++] = B64_TABLE[v & 0x3F];
    }
    // Remainder
    if (i < len) {
        int rem = len - i;
        uint32_t v = data[i] << 16;
        if (rem == 2) v |= data[i+1] << 8;
        out[o++] = B64_TABLE[(v >> 18) & 0x3F];
        out[o++] = B64_TABLE[(v >> 12) & 0x3F];
        out[o++] = (rem == 2) ? B64_TABLE[(v >> 6) & 0x3F] : '=';
        out[o++] = '=';
    }
    out[o] = '\0';
    *written = o;
    return 0;
}
#endif

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
#ifdef ESP32
    return mbedtls_base64_encode((unsigned char*) dst, dlen, olen, src, slen);
#elif ESP8266
    return base64_encode(src, slen, dst, dlen, olen);
#endif
}

inline void sha256Hash(const char *input, byte hash[32]) {
#ifdef ESP32
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);
    mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&md_ctx);
    mbedtls_md_update(&md_ctx, (const unsigned char*) input, strlen(input));
    mbedtls_md_finish(&md_ctx, hash);
    mbedtls_md_free(&md_ctx);
#elif ESP8266
#endif
}

inline int signRS256(const char *message, const char *privateKey, char *output, size_t outputSize) {
#ifdef ESP32
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
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    int ret = mbedtls_pk_parse_key(&pk, (const unsigned char*) privateKey,strlen(privateKey)+1, nullptr, 0, mbedtls_ctr_drbg_random, &ctr_drbg);
#else
    int ret = mbedtls_pk_parse_key(&pk, (const unsigned char*) privateKey,strlen(privateKey)+1, nullptr, 0);
#endif

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
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32,sig, sizeof(sig), &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);
#else
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32,sig, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);
#endif
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
#elif ESP8266
    br_sha256_context sha_ctx;
    br_sha256_init(&sha_ctx);
    br_sha256_update(&sha_ctx, message, strlen(message));
    uint8_t hash[32];
    br_sha256_out(&sha_ctx, hash);

    BearSSL::PrivateKey rsaKey(privateKey);
    if (!rsaKey.isRSA())
    {
        Serial.println("Provided key isn't RSA!");
        return -1;
    }

    static const uint8_t SHA256_OID[11] = {
        0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01
    };

    size_t sig_len = (rsaKey.getRSA()->n_bitlen+7)/8;
    uint8_t signature[sig_len];

    bool ok = EmberIotRSA::br_rsa_i31_pkcs1_sign(
      SHA256_OID,
      hash,
      sizeof(hash),
      rsaKey.getRSA(),
      signature
    );

    if (!ok) {
        Serial.println("RSA sign failed");
        return -1;
    }

    size_t written = 0;
    base64encode((unsigned char*) output, outputSize, &written, signature, sig_len);
    return 0;
#endif
}

#endif