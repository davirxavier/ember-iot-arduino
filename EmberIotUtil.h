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

#ifndef FIREBASE_UTIL_H
#define FIREBASE_UTIL_H

#ifdef ESP32
#include <WiFi.h>
#elif ESP8266
#include <ESP8266WiFi.h>
extern "C" {
#include <errno.h>
}
#endif

namespace FirePropUtil {
    inline size_t countOccurrences(const char *str, const char *sub) {
        int count = 0;
        const char *pos = str;
        int subLen = strlen(sub);

        if (subLen == 0) return 0; // Avoid infinite loop for empty substring

        while ((pos = strstr(pos, sub)) != nullptr) {
            count++;
            pos += subLen; // Move past the last found occurrence
        }

        return count;
    }

    inline void replaceSubstring(char *str, const char *oldWord, const char *newWord) {
        char *pos, *temp = str;
        size_t oldLen = strlen(oldWord);
        size_t newLen = strlen(newWord);

        size_t oldCount = countOccurrences(str, oldWord);
        char buffer[strlen(str) - (oldCount * oldLen) + (oldCount * newLen) + 16];
        buffer[0] = '\0';

        while ((pos = strstr(temp, oldWord)) != nullptr) {
            // Copy characters before the found substring
            strncat(buffer, temp, pos - temp);
            // Append the replacement string
            strcat(buffer, newWord);
            // Move past the old substring
            temp = pos + oldLen;
        }

        strcat(buffer, temp); // Append remaining part
        strcpy(str, buffer);  // Copy modified string back
    }

    inline bool endsWith(const char *str, const char *suffix)
    {
        if (!str || !suffix)
        {
            return true;
        }

        size_t lenstr = strlen(str);
        size_t lensuffix = strlen(suffix);

        if (lensuffix > lenstr)
        {
            return false;
        }

        return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
    }

    inline void setToNull(void **arr, size_t length)
    {
        for (size_t i = 0; i < length; i++) // No braces
            arr[i] = nullptr;
    }

    inline uint32_t fnv1aHash(const char* str) {
        uint32_t hash = 2166136261UL;  // FNV offset basis

        if (str == nullptr)
        {
            return hash;
        }

        while (*str) {
            hash ^= (uint8_t)(*str++);
            hash *= 16777619UL;  // FNV prime
        }
        return hash;
    }

    inline void initTime()
    {
#ifdef ESP32
        configTime(0, 0, "pool.ntp.org");
#endif
    }

    inline bool isTimeInitialized()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return false;
        }

        tm time;
        return getLocalTime(&time);
    }


    typedef enum {
        STR2INT_SUCCESS,
        STR2INT_OVERFLOW,
        STR2INT_UNDERFLOW,
        STR2INT_INCONVERTIBLE
    } str2int_errno;

    /* Convert string s to int out.
     *
     * @param[out] out The conv erted int. Cannot be NULL.
     *
     * @param[in] s Input string to be converted.
     *
     *     The format is the same as strtol,
     *     except that the following are inconvertible:
     *
     *     - empty string
     *     - leading whitespace
     *     - any trailing characters that are not part of the number
     *
     *     Cannot be NULL.
     *
     * @param[in] base Base to interpret string in. Same range as strtol (2 to 36).
     *
     * @return Indicates if the operation succeeded, or why it failed.
     */
    inline str2int_errno str2int(int *out, char *s, int base) {
        char *end;
        if (s[0] == '\0' || isspace((unsigned char) s[0]))
            return STR2INT_INCONVERTIBLE;
        errno = 0;
        long l = strtol(s, &end, base);
        /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
        if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
            return STR2INT_OVERFLOW;
        if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
            return STR2INT_UNDERFLOW;
        if (*end != '\0')
            return STR2INT_INCONVERTIBLE;
        *out = l;
        return STR2INT_SUCCESS;
    }

    inline str2int_errno str2ul(unsigned long *out, char *s, int base) {
        char *end;
        if (s[0] == '\0' || isspace((unsigned char) s[0]))
            return STR2INT_INCONVERTIBLE;
        errno = 0;
        unsigned long l = strtoul(s, &end, base);
        if (errno == ERANGE && l == ULONG_MAX)
            return STR2INT_OVERFLOW;
        if (*end != '\0')
            return STR2INT_INCONVERTIBLE;
        *out = l;
        return STR2INT_SUCCESS;
    }
}

#endif //FIREBASE_UTIL_H
