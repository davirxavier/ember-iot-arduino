//
// Created by xav on 3/27/25.
//

#ifndef FIREBASERTDBSTREAM_H
#define FIREBASERTDBSTREAM_H

#include <EmberIotHttp.h>
#include <EmberIotUtil.h>
#include <WithSecureClient.h>

#define EMBER_STREAM_MAXIMUM_REDIRECTS 5

namespace EmberIotStreamValues
{
    const char AUTH_PARAM[] PROGMEM = "?auth=";
    const uint8_t AUTH_PARAM_SIZE = strlen_P(AUTH_PARAM);

    const char PROTOCOL[] PROGMEM = "https://";
    const uint8_t PROTOCOL_SIZE = strlen_P(PROTOCOL);

    const char DATA_HEADER[] PROGMEM = "data:";
    const char EVENT_HEADER[] PROGMEM = "event:";
    const char CANCEL_EVENT[] PROGMEM = "cancel";
    const char AUTH_REVOKED_EVENT[] PROGMEM = "auth_revoked";

    const char LAST_SEEN_BODY[] PROGMEM = R"({"last_seen":)";
}

/**
 * Callback for where there's new data to be processed in stream. Stream is the full raw update data received.
 */
typedef std::function<void(Stream &stream)> RTDBStreamCallback;

class EmberIotStream : public WithSecureClient
{
public:

    EmberIotStream(EmberIotAuth *auth, const char *host, const char *path) : host(host), auth(auth)
    {
        isStarted = false;
        isUidReplaced = false;
        updateCallback = nullptr;
        lastConnection = 0;
        lastUpdate = 0;
        updateInterval = 750;
        lastKeepAlive = 0;

        bool endsWithJson = FirePropUtil::endsWith(path, ".json");

        this->path = new char[strlen(path) + 1 + (endsWithJson ? 0 : 5)]{};
        strcpy(this->path, path);

        if (!endsWithJson)
        {
            strcat(this->path, ".json");
        }
    }

    void setCallback(RTDBStreamCallback cb)
    {
        this->updateCallback = cb;
    }

    bool hasAuth() const
    {
        return this->auth != nullptr;
    }

    char *getPath()
    {
        return path;
    }

    void start()
    {
        if (isStarted)
        {
            return;
        }
        isStarted = true;
        lastConnection = millis()-5000;
    }

    void stop()
    {
        if (!isStarted)
        {
            return;
        }

        isStarted = false;
        client.stop();
    }

    void loop()
    {
        if (!isStarted || (auth != nullptr && strlen(auth->getUserUid()) == 0))
        {
            return;
        }

        if (!isUidReplaced)
        {
            size_t occurrences = FirePropUtil::countOccurrences(this->path, "$uid");

            if (occurrences > 0)
            {
                char *uid = auth->getUserUid();
                size_t uidLen = strlen(uid);

                size_t bufSize = strlen(this->path) - (occurrences * 4) + (occurrences * uidLen) + 1;
                char buf[bufSize];
                strcpy(buf, this->path);

                delete[] this->path;
                this->path = new char[bufSize]{};
                strcpy(this->path, buf);

                FirePropUtil::replaceSubstring(this->path, "$uid", auth->getUserUid());
            }

            isUidReplaced = true;
            return;
        }

        bool isConnected = client.connected();
        if (isConnected && millis() - lastUpdate > updateInterval)
        {
            handleUpdate();
            lastUpdate = millis();
        }
        else if (!isConnected && millis() - lastConnection > 5000)
        {
            EMBER_PRINT_MEM("Memory while stream disconnected");
            HTTP_LOGF("Client for %s has disconnected from stream, trying to reconnect.\n", path);

            if (connect())
            {
                delay(2000);
                HTTP_LOGN("Reconnected.");
            }
            else
            {
                HTTP_LOGN("Reconnection failed, retrying.");
                client.stop();
            }

            lastConnection = millis();
        }
    }

    bool isConnected()
    {
        return client.connected();
    }

    unsigned long updateInterval;
private:
    bool connect()
    {
        if (!HTTP_UTIL::connectToHost(host, client)) {
            HTTP_LOGN("Failed to connect to host.");
            return false;
        }

        HTTP_UTIL::printHttpMethod(FPSTR(HTTP_UTIL::METHOD_GET), client);
        HTTP_PRINT_BOTH(path, client);
        if (hasAuth())
        {
            HTTP_PRINT_BOTH(FPSTR(EmberIotStreamValues::AUTH_PARAM), client);
            auth->writeToken(client);
        }
        HTTP_UTIL::printHttpVer(client);
        HTTP_UTIL::printHost(host, client);
        HTTP_PRINT_BOTH(F("Accept: text/event-stream"), client);
        HTTP_PRINT_LN(client);
        HTTP_PRINT_BOTH(F("Connection: keep-alive"), client);
        HTTP_PRINT_LN(client);
        HTTP_PRINT_LN(client);

        uint8_t redirectionCount = 0;
        while (redirectionCount++ <= EMBER_STREAM_MAXIMUM_REDIRECTS)
        {
            const char *search[] = {"location: http", "\r\n\r\n", "\n\n"};
            uint8_t found = HTTP_UTIL::findFirstSkipWhitespace(client, search, 3, true, true);

            if (found != 0)
            {
                break;
            }

            HTTP_LOGN("Location header found, redirecting.");
            char locationBuffer[256];
            size_t read = client.readBytesUntil('\n', locationBuffer, sizeof(locationBuffer)-1);
            locationBuffer[read < sizeof(locationBuffer)-1 ? read : sizeof(locationBuffer)-1] = 0;

            HTTP_LOGF("Location header: %s\n", locationBuffer);

            if (read > 1 && locationBuffer[read-1] == '\r')
            {
                locationBuffer[read-1] = 0;
            }

            if (locationBuffer[0] != 's')
            {
                HTTP_LOGN("Location header value is not https, this is not supported, cancelling.");
                client.stop();
                return false;
            }

            char *firstSlash = strchr(locationBuffer, '/');
            if (firstSlash != nullptr) {
                firstSlash[0] = 0;
                firstSlash++;
                HTTP_LOGF("Extracted uri: /%s\n", firstSlash);
            }

            const char *newHost = locationBuffer[0] == 0 ? host : locationBuffer+4;
            client.stop();

            if (!HTTP_UTIL::connectToHost(newHost, client)) {
                HTTP_LOGN("Failed to connect to host.");
                return false;
            }

            HTTP_UTIL::printHttpMethod(FPSTR(HTTP_UTIL::METHOD_GET), client);
            HTTP_PRINT_BOTH("/", client);
            HTTP_PRINT_BOTH(firstSlash != nullptr ? firstSlash : "", client);
            if (hasAuth())
            {
                HTTP_PRINT_BOTH(FPSTR(EmberIotStreamValues::AUTH_PARAM), client);
                auth->writeToken(client);
            }
            HTTP_UTIL::printHttpVer(client);

            HTTP_UTIL::printHost(newHost, client);
            HTTP_PRINT_BOTH(F("Accept: text/event-stream"), client);
            HTTP_PRINT_LN(client);
            HTTP_PRINT_BOTH(F("Connection: keep-alive"), client);
            HTTP_PRINT_LN(client);
            HTTP_PRINT_LN(client);
        }

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        if (!HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGF("Error while trying to start stream: %d\n", responseStatus);
            return false;
        }

        return true;
    }

    void handleUpdate()
    {
        if (updateCallback == nullptr)
        {
            return;
        }

        unsigned int currentEventChar = 0;
        unsigned int eventHeaderLength = strlen_P(EmberIotStreamValues::EVENT_HEADER);
        char eventHeader[eventHeaderLength+1];
        strcpy_P(eventHeader, EmberIotStreamValues::EVENT_HEADER);

        unsigned int currentDataChar = 0;
        unsigned int dataHeaderLength = strlen_P(EmberIotStreamValues::DATA_HEADER);
        char dataHeader[dataHeaderLength+1];
        strcpy_P(dataHeader, EmberIotStreamValues::DATA_HEADER);

        if (client.available())
        {
            EMBER_PRINT_MEM("Memory while stream connected and has data");
        }

        while (client.available()) {
            char c = tolower(client.read());

            if (c == eventHeader[currentEventChar])
            {
                currentEventChar++;
            }
            else
            {
                currentEventChar = 0;
            }

            if (currentEventChar >= eventHeaderLength)
            {
                if (client.peek() == ' ')
                {
                    client.read();
                }

                size_t bufSize = 65;
                char eventBuf[bufSize];
                size_t read = client.readBytesUntil('\n', eventBuf, bufSize-1);
                eventBuf[read < bufSize-1 ? read : bufSize-1] = 0;
                HTTP_LOGF("Event header value: %s\n", eventBuf);

                if (strcmp_P(eventBuf, EmberIotStreamValues::CANCEL_EVENT) == 0 || strcmp_P(eventBuf, EmberIotStreamValues::AUTH_REVOKED_EVENT) == 0)
                {
                    HTTP_LOGN("Cancel or auth revoked event received, disconnecting stream.");
                    client.stop();
                    return;
                }
            }

            if (c == dataHeader[currentDataChar])
            {
                currentDataChar++;
            }
            else
            {
                currentDataChar = 0;
            }

            if (currentDataChar >= dataHeaderLength)
            {
                updateCallback(client);
            }
        }
    }

    bool isStarted;
    bool isUidReplaced;
    const char *host;
    char *path;
    EmberIotAuth *auth;
    unsigned long lastConnection;
    unsigned long lastUpdate;
    unsigned long lastKeepAlive;
    RTDBStreamCallback updateCallback;
};

#endif //FIREBASERTDBSTREAM_H
