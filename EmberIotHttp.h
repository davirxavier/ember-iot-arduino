//
// Created by xav on 3/27/25.
//

#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

#include <EmberIotUtil.h>
#include <WiFiClientSecure.h>

#ifdef EMBER_ENABLE_LOGGING
#define HTTP_LOG(str) Serial.print(str)
#define HTTP_LOGN(str) Serial.print("[EMBER-IOT-HTTP] "); Serial.println(str)
#define HTTP_LOGF(str, p...) Serial.print("[EMBER-IOT-HTTP] "); Serial.printf(str, p)
#else
#define HTTP_LOGN(str)
#define HTTP_LOGF(str, p...)
#endif

#ifndef EMBER_HTTP_BUFFER_SIZE
#define EMBER_HTTP_BUFFER_SIZE 64
#endif

#ifdef EMBER_ENABLE_DEBUG_LOG
#define EMBER_DEBUG(str) Serial.print(str)
#define EMBER_DEBUGN(str) Serial.print("[EMBER-IOT-DEBUG] "); Serial.println(str)
#define EMBER_DEBUGF(str, p...) Serial.print("[EMBER-IOT-DEBUG] "); Serial.printf(str, p)

#ifdef ESP32
#define EMBER_PRINT_MEM(str) Serial.print("[EMBER-IOT-DEBUG] "); EMBER_DEBUG(str); Serial.printf(" (free/total): %lu/%lu\n", esp_get_free_heap_size(), heap_caps_get_total_size(MALLOC_CAP_8BIT))
#elif ESP8266
#define EMBER_PRINT_MEM(str) Serial.print("[EMBER-IOT-DEBUG] "); EMBER_DEBUG(str); Serial.printf(" (free): %du\n", ESP.getFreeHeap())
#endif

#else
#define EMBER_DEBUG(str)
#define EMBER_DEBUGN(str)
#define EMBER_DEBUGF(pattern, p...)
#define EMBER_PRINT_MEM(str)
#endif

#define HTTP_PRINT_LN(stream) EMBER_DEBUGN(""); stream.print("\r\n")
#define HTTP_PRINT_BOTH(val, stream) EMBER_DEBUG(val); stream.print(val)
#define HTTP_PRINT_BOTH(val, stream) EMBER_DEBUG(val); stream.print(val)
#define HTTP_PRINT_BOTH_2(val) EMBER_DEBUG(val); client.print(val)

namespace HTTP_UTIL
{
    typedef uint8_t buffer_t[EMBER_HTTP_BUFFER_SIZE];

    const char* METHOD_POST PROGMEM = "POST ";
    const char* METHOD_GET PROGMEM = "GET ";
    const char* METHOD_PUT PROGMEM = "PUT ";
    const char* METHOD_PATCH PROGMEM = "PATCH ";
    const char* METHOD_DELETE PROGMEM = "DELETE ";

    const char* LOCATION_HEADER PROGMEM = "location:";
    const char* HTTP_VER PROGMEM = " HTTP/1.1";

    inline bool isSuccess(int statusCode)
    {
        return statusCode >= 200 && statusCode < 300;
    }

    inline int getStatusCode(WiFiClientSecure &client)
    {
        EMBER_DEBUG("Reading status code: ");
        char code[4]{0};
        uint8_t currentCodeChar = 0;

        bool spaceFound = false;
        bool hasRead = false;
        while (client.connected())
        {
            if (client.available())
            {
                hasRead = true;
                char c = client.read();
                EMBER_DEBUG(c);
                bool isSpace = isspace((unsigned char) c);
                if (isSpace && !spaceFound)
                {
                    spaceFound = true;
                }

                if (spaceFound && isSpace)
                {
                    continue;
                }

                if (spaceFound)
                {
                    EMBER_DEBUG(c);
                    code[currentCodeChar] = c;
                    currentCodeChar++;

                    if (currentCodeChar >= 3)
                    {
                        code[currentCodeChar] = 0;
                        break;
                    }
                }
            }
            else if (hasRead)
            {
                break;
            }
        }
        EMBER_DEBUGN("");
        EMBER_DEBUGF("Status code string: %s\n", code);
        if (currentCodeChar < 3)
        {
            return -1;
        }

        client.find("\n");

        int ret = -1;
        if (FirePropUtil::str2int(&ret, code, 10) != FirePropUtil::STR2INT_SUCCESS)
        {
            return -1;
        }

        EMBER_DEBUGF("Parsed status code: %d\n", ret);
        return ret;
    }

    inline void disconnect(WiFiClientSecure &client)
    {
        client.stop();
#ifdef ESP32
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        client.clear();
#else
        client.flush();
#endif
#endif
    }

    inline bool connectToHost(const char *hostname, WiFiClientSecure &client)
    {
        EMBER_DEBUGF("New https request for host: %s\n", hostname);
        disconnect(client);

#ifdef EMBER_HTTP_INSECURE
        client.setInsecure();
#endif

        if (!client.connect(hostname, 443))
        {
            EMBER_DEBUGN("Connection to host failed.");
            return false;
        }
        return true;
    }

    inline void printHttpMethod(const __FlashStringHelper *method, WiFiClientSecure &client)
    {
        HTTP_PRINT_BOTH(method, client);
    }

    inline void printHttpVer(WiFiClientSecure &client)
    {
        HTTP_PRINT_BOTH(FPSTR(HTTP_VER), client);
        EMBER_DEBUGN();
        HTTP_PRINT_LN(client);
    }

    inline void printHttpProtocol(const __FlashStringHelper *path, const __FlashStringHelper *method, WiFiClientSecure &client)
    {
        printHttpMethod(method, client);
        HTTP_PRINT_BOTH(path, client);
        printHttpVer(client);
    }

    inline void printHost(const char *host, WiFiClientSecure &client)
    {
        HTTP_PRINT_BOTH(F("Host: "), client);
        HTTP_PRINT_BOTH(host, client);
        EMBER_DEBUGN();
        HTTP_PRINT_LN(client);
    }

    inline void printContentType(WiFiClientSecure &client, const char *contentType = "application/json")
    {
        HTTP_PRINT_BOTH(F("Content-Type: "), client);
        HTTP_PRINT_BOTH(contentType, client);
        EMBER_DEBUGN();
        HTTP_PRINT_LN(client);
    }

    inline void printContentLengthAndEndHeaders(unsigned long contentLength, WiFiClientSecure &client)
    {
        HTTP_PRINT_BOTH(F("Content-Length: "), client);
        HTTP_PRINT_BOTH(contentLength, client);
        EMBER_DEBUGN();
        HTTP_PRINT_LN(client);
        HTTP_PRINT_LN(client);
    }

    /**
     * Write from input to output until terminator is found (exclusive).
     */
    inline bool printChunkedUntil(Stream &input, Stream &output, const char *terminator)
    {
        size_t currentTerminatorChar = 0;
        size_t terminatorLength = strlen(terminator);
        const size_t bufferSize = EMBER_HTTP_BUFFER_SIZE;

        uint8_t readBuf[bufferSize];
        uint8_t writeBuf[bufferSize];

        while (input.available())
        {
            size_t read = input.readBytes(readBuf, bufferSize);

            for (size_t i = 0; i < read; i++)
            {
                char c = readBuf[i];
                writeBuf[i] = c;

                if (terminator[currentTerminatorChar] == c)
                {
                    currentTerminatorChar++;

                    if (currentTerminatorChar >= terminatorLength)
                    {
                        output.write(writeBuf, i);
                        return true;
                    }
                }
                else
                {
                    currentTerminatorChar = 0;
                }
            }

            output.write(writeBuf, read);
        }

        return false;
    }

    inline void printChunked(Stream &input, Stream &output)
    {
        uint8_t buf[EMBER_HTTP_BUFFER_SIZE];
        while (input.available())
        {
            size_t read = input.readBytes(buf, EMBER_HTTP_BUFFER_SIZE);
            output.write(buf, read);
        }
    }

    inline bool findSkipWhitespace(Stream &stream, const char *terminator, bool ignoreCase = false, bool skipOnlySpaces = false)
    {
        size_t terminatorLength = strlen(terminator);
        size_t currentTerminatorChar = 0;

        while (stream.available())
        {
            char c = ignoreCase ? tolower(stream.read()) : stream.read();

            if (skipOnlySpaces)
            {
                if (c == ' ')
                {
                    continue;
                }
            }
            else if (isspace((unsigned char) c))
            {
                continue;
            }

            if ((ignoreCase ? tolower(terminator[currentTerminatorChar]) : terminator[currentTerminatorChar]) == c)
            {
                currentTerminatorChar++;

                if (currentTerminatorChar >= terminatorLength)
                {
                    return true;
                }
            }
            else
            {
                currentTerminatorChar = 0;
            }
        }

        return false;
    }

    /**
     * Reads stream until the first occurence of any of the passed strings, ignoring any whitespace.
     *
     * @param stream
     * @param terminators List of strings to search for.
     * @param arrayLength Length of the string array.
     * @return Index (0-based) of the first terminator found, or -1 if none are found.
     */
    inline int findFirstSkipWhitespace(Stream &stream, const char **terminators, size_t arrayLength, bool ignoreCase = false, bool skipOnlySpaces = false)
    {
        size_t terminatorLengths[arrayLength];
        size_t currentTerminatorChars[arrayLength];

        for (size_t i = 0; i < arrayLength; i++)
        {
            terminatorLengths[i] = strlen(terminators[i]);
            currentTerminatorChars[i] = 0;
        }

        while (stream.available())
        {
            int r = stream.read();
            char c = ignoreCase ? tolower(r) : r;

            if (skipOnlySpaces)
            {
                if (c == ' ')
                {
                    continue;
                }
            }
            else if (isspace((unsigned char) c))
            {
                continue;
            }

            for (size_t i = 0; i < arrayLength; i++)
            {
                const char *terminator = terminators[i];

                if ((ignoreCase ? tolower(terminator[currentTerminatorChars[i]]) : terminator[currentTerminatorChars[i]]) == c)
                {
                    currentTerminatorChars[i]++;

                    if (currentTerminatorChars[i] >= terminatorLengths[i])
                    {
                        return i;
                    }
                }
                else
                {
                    currentTerminatorChars[i] = 0;
                }
            }
        }

        return -1;
    }
}

#endif //HTTP_UTIL_H
