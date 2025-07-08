//
// Created by xav on 3/27/25.
//

#ifndef HTTP_UTIL_H
#define HTTP_UTIL_H

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#ifdef EMBER_ENABLE_LOGGING
#define HTTP_LOGN(str) Serial.println(str)
#define HTTP_LOGF(str, p...) Serial.printf(str, p)
#else
#define HTTP_LOGN(str)
#define HTTP_LOGF(str, p...)
#endif

namespace HTTP_UTIL
{
    const char* METHOD_POST PROGMEM = "POST ";
    const char* METHOD_GET PROGMEM = "GET ";
    const char* METHOD_PUT PROGMEM = "PUT ";
    const char* METHOD_PATCH PROGMEM = "PATCH ";
    const char* METHOD_DELETE PROGMEM = "DELETE ";

    const char* LOCATION_HEADER PROGMEM = "location:";
    const char* HTTP_VER PROGMEM = " HTTP/1.1";

    inline void printContentLength(WiFiClientSecure &client, unsigned long contentLength, char *buffer, size_t bufferSize)
    {
        snprintf(buffer, bufferSize, "Content-Length: %lu", contentLength);
        client.println(buffer);
        client.println();
    }

    inline int getStatusCode(const uint8_t *buf, uint8_t length)
    {
        if (length < 13)
        {
            return -1;
        }

        char status[4];
        strncpy(status, (char*) buf+9, 3);
        status[3] = 0;
        return atoi(status);
    }

    inline bool doChunkedHttpRequest(WiFiClientSecure& client,
                                  const char* url,
                                  const char* hostname,
                                  const __FlashStringHelper *method,
                                  std::function<void(WiFiClientSecure& client)> sendBodyFn,
                                  std::function<bool(const uint8_t buf[64], uint8_t length)> callbackFn,
                                  const char* contentType = nullptr)
    {
        if (!client.connect(hostname, 443))
        {
            HTTP_LOGN("Connection to host failed.");
            return false;
        }

        unsigned int bufferSize = max(256U, strlen(url) + strlen_P((PGM_P) method) + strlen_P(HTTP_UTIL::HTTP_VER) + 1);
        char headerBuffer[bufferSize];

        strncpy_P(headerBuffer, (PGM_P) method, bufferSize);
        strcat(headerBuffer, url);
        strcat_P(headerBuffer, HTTP_VER);
        client.println(headerBuffer);

        snprintf(headerBuffer, bufferSize, "Host: %s", hostname);
        client.println(headerBuffer);

        if (contentType == nullptr)
        {
            client.println(F("Content-Type: application/json"));
        }
        else
        {
            client.println(contentType);
        }

        sendBodyFn(client);

        uint8_t buf[64];
        while (client.connected())
        {
            if (client.available())
            {
                size_t readLength = client.readBytes(buf, 64);
                bool continueReading = callbackFn(buf, readLength);
                if (!continueReading)
                {
                    break;
                }
            }
        }
        client.stop();
        return true;
    }

    inline bool doJsonHttpRequest(WiFiClientSecure& client,
                                  const char* url,
                                  const char* hostname,
                                  const __FlashStringHelper *method,
                                  const char* requestBody,
                                  JsonDocument* filter = nullptr,
                                  JsonDocument* responseDoc = nullptr,
                                  const char* contentType = nullptr)
    {
        if (!client.connect(hostname, 443))
        {
            HTTP_LOGN("Connection to host failed.");
            return false;
        }

        unsigned int bufferSize = max(256U, strlen(url) + strlen_P((PGM_P) method) + strlen_P(HTTP_UTIL::HTTP_VER) + 1);
        char headerBuffer[bufferSize];

        strncpy_P(headerBuffer, (PGM_P) method, bufferSize);
        strcat(headerBuffer, url);
        strcat_P(headerBuffer, HTTP_VER);
        client.println(headerBuffer);

        snprintf(headerBuffer, bufferSize, "Host: %s", hostname);
        client.println(headerBuffer);

        if (contentType == nullptr)
        {
            client.println(F("Content-Type: application/json"));
        }
        else
        {
            client.println(contentType);
        }

        snprintf(headerBuffer, bufferSize, "Content-Length: %d", strlen(requestBody));
        client.println(headerBuffer);

        client.println();
        client.print(requestBody);

        String response;
        while (client.connected())
        {
            if (client.available())
            {
                response = client.readString();
                break;
            }
        }
        client.stop();

        if (!response.startsWith(F("HTTP/1.1 2")))
        {
            HTTP_LOGF("HTTP response is not OK: %s\n", response.c_str());
            return false;
        }

        if (responseDoc == nullptr)
        {
            return true;
        }

        int bodyStartIndex = response.indexOf("\r\n\r\n");
        bodyStartIndex = bodyStartIndex != -1 ? bodyStartIndex + 4 : 0;
        bodyStartIndex = response.indexOf("\r\n", bodyStartIndex);
        bodyStartIndex = bodyStartIndex != -1 ? bodyStartIndex + 2 : 0;

        const DeserializationError error = filter != nullptr
                                               ? deserializeJson(*responseDoc,
                                                                 response.substring(bodyStartIndex),
                                                                 DeserializationOption::Filter(*filter))
                                               : deserializeJson(*responseDoc, response.substring(bodyStartIndex));
        if (error)
        {
            HTTP_LOGF("JSON response parse error: %s\n", error.c_str());
            return false;
        }

        return true;
    }

    inline int connectToTextStream(WiFiClientSecure& client,
                                   const char* url,
                                   const char* hostname,
                                   const __FlashStringHelper *method,
                                   uint8_t maxRedirection = 10,
                                   uint8_t currentRedirection = 0)
    {
        HTTP_LOGF("Connecting to text stream at: %s\n", url);
        if (!client.connect(hostname, 443))
        {
            Serial.println("Connection failed!");
            return -1;
        }
        HTTP_LOGN("Connected!");

        unsigned int bufferSize = max(256U, strlen(url) + strlen_P((PGM_P) method) + strlen_P(HTTP_UTIL::HTTP_VER) + 1);
        char headerBuffer[bufferSize];
        headerBuffer[0] = 0;

        strncpy_P(headerBuffer, (PGM_P) method, bufferSize);
        strcat(headerBuffer, url);
        strcat_P(headerBuffer, HTTP_VER);
        client.println(headerBuffer);

        snprintf(headerBuffer, bufferSize, "Host: %s", hostname);
        client.println(headerBuffer);

        client.println(F("Accept: text/event-stream"));
        client.println(F("Connection: keep-alive"));
        client.println();

        HTTP_LOGN("Looking for status and location header.");
        unsigned int locationSize = strlen_P(LOCATION_HEADER);
        char locationHeader[locationSize+1];
        strcpy_P(locationHeader, LOCATION_HEADER);

        unsigned int currentLocationChar = 0;
        int status = -1;

        while (client.connected())
        {
            if (client.available())
            {
                client.read();
                if (status == -1)
                {
                    client.readBytesUntil('\n', headerBuffer, 255);

                    char* firstSpace = strchr(headerBuffer, ' ');
                    if (firstSpace == nullptr)
                    {
                        return -1;
                    }

                    firstSpace++;
                    char* lastSpace = strchr(firstSpace + 1, ' ');
                    if (lastSpace == nullptr)
                    {
                        return -1;
                    }

                    status = strtoul(firstSpace, &lastSpace, 10);
                    HTTP_LOGF("Stream connect HTTP response status: %d.\n", status);
                    if (status != 200)
                    {
                        return status;
                    }
                }

                const char c = tolower(client.read());

                if (currentLocationChar < locationSize && c == locationHeader[currentLocationChar])
                {
                    currentLocationChar++;
                }
                else
                {
                    currentLocationChar = 0;
                }

                if (currentLocationChar >= locationSize)
                {
                    String location = client.readStringUntil('\n');
                    location.trim();

                    HTTP_LOGF("Location header found, redirecting to: %s\n", location.c_str());
                    client.stop();

                    if (currentRedirection > maxRedirection)
                    {
                        HTTP_LOGN("Redirection limit reached, stopping connection.");
                        return -1;
                    }

                    if (location.startsWith("https"))
                    {
                        headerBuffer[0] = 0;
                        strcpy(headerBuffer, location.c_str() + 8);

                        unsigned int newHostLength = strlen(headerBuffer);
                        for (unsigned int i = 0; i < newHostLength; i++)
                        {
                            if (headerBuffer[i] == '/')
                            {
                                headerBuffer[i] = 0;
                                break;
                            }
                        }

                        return connectToTextStream(client, location.c_str(), headerBuffer, method, maxRedirection,
                                                   currentRedirection + 1);
                    }
                    else
                    {
                        snprintf(headerBuffer, bufferSize, location.startsWith("/") ? "https://%s%s" : "https://%s/%s",
                                 hostname,
                                 location.c_str());
                        return connectToTextStream(client, headerBuffer, hostname, method, maxRedirection,
                                                   currentRedirection + 1);
                    }
                }
            }
            else
            {
                break;
            }
        }

        HTTP_LOGN("No redirection found, connection successful.");
        return 200;
    }
}

#endif //HTTP_UTIL_H
