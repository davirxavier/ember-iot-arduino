//
// Created by xav on 3/27/25.
//

#ifndef FIREBASERTDBSTREAM_H
#define FIREBASERTDBSTREAM_H

#include <EmberIotHttp.h>
#include <EmberIotUtil.h>
#include <WithSecureClient.h>

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

    const char LAST_SEEN_PATH[] PROGMEM = "last_seen";
    const uint8_t LAST_SEEN_PATH_SIZE = strlen_P(LAST_SEEN_PATH);
}

typedef void (*RTDBStreamCallback)(const char *data);

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
        if (!isStarted || (auth != nullptr && auth->getUserUid() == nullptr))
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
        char url[EmberIotStreamValues::PROTOCOL_SIZE + strlen(host) + strlen(path) + (hasAuth() ? auth->getTokenSize() + EmberIotStreamValues::AUTH_PARAM_SIZE : 0) + 1];
        url[0] = 0;

        if (hasAuth())
        {
            sprintf_P(url, PSTR("%S%s%s%S%s"), EmberIotStreamValues::PROTOCOL, this->host, this->path, EmberIotStreamValues::AUTH_PARAM, auth->getToken());
        }
        else
        {
            sprintf_P(url, PSTR("%S%s%s"), EmberIotStreamValues::PROTOCOL, this->host, this->path);
        }

        return HTTP_UTIL::connectToTextStream(client, url, this->host, FPSTR(HTTP_UTIL::METHOD_GET)) == 200;
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

                char eventBuf[65];
                eventBuf[client.readBytesUntil('\n', eventBuf, 64)] = 0;
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
                String line = client.readStringUntil('\n');
                line.trim();

                if (!line.isEmpty() && !line.equalsIgnoreCase("null"))
                {
                    updateCallback(line.c_str());
                }
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
