//
// Created by xav on 3/28/25.
//

#ifndef FIREPROP_H
#define FIREPROP_H

#include <EmberIotShared.h>
#include <EmberIotUtil.h>
#include <WithSecureClient.h>
#include <EmberIotAuth.h>
#include <EmberIotStream.h>
#include <time.h>

#define UPDATE_LAST_SEEN_INTERVAL 60000
#define EMBER_BUTTON_OFF 0
#define EMBER_BUTTON_ON 1
#define EMBER_BUTTON_PUSH 2

class EmberIot : WithSecureClient
{
public:
    EmberIot(const char* dbUrl,
             const char* deviceId,
             const unsigned int &boardId = 0,
             const char* username = nullptr,
             const char* password = nullptr,
             const char* webApiKey = nullptr) : dbUrl(dbUrl)
    {
        stream = nullptr;
        inited = false;
        path = nullptr;
        auth = nullptr;
        lastUpdatedChannels = 0;
        lastHeartbeat = -UPDATE_LAST_SEEN_INTERVAL;
        snprintf(EmberIotChannels::boardId, sizeof(EmberIotChannels::boardId), "%d", boardId);

        if (username != nullptr && password != nullptr && webApiKey != nullptr)
        {
            auth = new EmberIotAuth(username, password, webApiKey);
        }

        const size_t pathSize = strlen(EMBERIOT_STREAM_PATH) + strlen(deviceId) + strlen(EMBERIOT_PROP_PATH) + 2;
        path = new char[pathSize]{};
        snprintf(path, pathSize, "%s%s/%s", EMBERIOT_STREAM_PATH, deviceId, EMBERIOT_PROP_PATH);
        stream = new EmberIotStream(auth, dbUrl, path);
        stream->setCallback(EmberIotChannels::streamCallback);

        for (size_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            EmberIotChannels::callbacks[i] = nullptr;
            hasUpdateByChannel[i] = false;
            updateDataByChannel[i][0] = 0;
        }
    }

    void init()
    {
        stream->start();
        inited = true;
        EmberIotChannels::started = true;
        configTime(0, 0, "pool.ntp.org");
    }

    void loop()
    {
        if (!inited)
        {
            return;
        }

        auth->loop();
        stream->loop();

        if (millis() - lastHeartbeat > UPDATE_LAST_SEEN_INTERVAL)
        {
            writeLastSeen();
            lastHeartbeat = millis();
        }

        if (millis() - lastUpdatedChannels < 500)
        {
            return;
        }

        uint8_t updateCount = 0;
        for (const bool i : hasUpdateByChannel)
        {
            if (i)
            {
                updateCount++;
            }
        }

        if (updateCount > 0)
        {
            JsonDocument doc;

            for (uint8_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
            {
                if (!hasUpdateByChannel[i])
                {
                    continue;
                }

                char buf[5];
                sprintf(buf, "CH%d", i);
                doc[buf]["d"] = updateDataByChannel[i];
                doc[buf]["w"] = EmberIotChannels::boardId;
            }

            unsigned int bodySize = measureJson(doc)+2;
            char body[bodySize];
            serializeJson(doc, body, bodySize);

            bool result = write(body);
            if (result)
            {
                for (bool & i : hasUpdateByChannel)
                {
                    i = false;
                }
            }
            else
            {
                HTTP_LOGN("Error while trying to send data to server, retrying shortly.");
            }
        }

        lastUpdatedChannels = millis();
    }

    void channelWrite(uint8_t channel, const char* value)
    {
        hasUpdateByChannel[channel] = true;
        strncpy(updateDataByChannel[channel], value, EMBER_MAXIMUM_STRING_SIZE);
        updateDataByChannel[channel][EMBER_MAXIMUM_STRING_SIZE] = 0;
    }

    void channelWrite(uint8_t channel, int value)
    {
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%d", value);
    }

    void channelWrite(uint8_t channel, double value)
    {
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%f", value);
    }

    void channelWrite(uint8_t channel, long long value)
    {
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%lld", value);
    }

private:
    bool write(const char* data)
    {
        size_t bufSize = strlen(EmberIotStreamValues::PROTOCOL) +
            strlen(dbUrl) +
            strlen(EmberIotStreamValues::AUTH_PARAM) +
            strlen(stream->getPath()) +
            (auth != nullptr ? auth->getTokenSize() : 0) + 8;

        size_t pathSize = strlen(stream->getPath());
        char finalPath[pathSize+1];
        if (FirePropUtil::endsWith(stream->getPath(), ".json"))
        {
            strcpy(finalPath, stream->getPath());
            finalPath[pathSize-5] = 0;
        }

        char buf[bufSize];
        sprintf(buf, "%s%s%s.json%s%s&print=silent",
                EmberIotStreamValues::PROTOCOL,
                dbUrl,
                finalPath,
                EmberIotStreamValues::AUTH_PARAM,
                auth != nullptr ? auth->getToken() : "");

        HTTP_LOGF("Setting properties in path: %s, body:\n%s\n", buf, data);

        return HTTP_UTIL::doJsonHttpRequest(client,
                                            buf,
                                            dbUrl,
                                            HTTP_UTIL::METHOD_PATCH,
                                            data);
    }

    bool writeLastSeen()
    {
        size_t bufSize = strlen(EmberIotStreamValues::PROTOCOL) +
            strlen(dbUrl) +
            strlen(EmberIotStreamValues::AUTH_PARAM) +
            strlen(stream->getPath()) +
            (auth != nullptr ? auth->getTokenSize() : 0) + 8;

        size_t pathSize = strlen(stream->getPath());
        char finalPath[pathSize+EmberIotStreamValues::LAST_SEEN_PATH_SIZE+1];
        finalPath[0] = 0;

        char *lastSlash = strrchr(stream->getPath(), '/');
        if (lastSlash != nullptr)
        {
            size_t index = lastSlash - stream->getPath();
            strcpy(finalPath, stream->getPath());
            finalPath[index] = 0;
        }
        else
        {
            strcpy(finalPath, stream->getPath());
        }

        char buf[bufSize];
        sprintf(buf, "%s%s%s.json%s%s&print=silent",
                EmberIotStreamValues::PROTOCOL,
                dbUrl,
                finalPath,
                EmberIotStreamValues::AUTH_PARAM,
                auth != nullptr ? auth->getToken() : "");

        time_t now;
        tm timeinfo;
        getLocalTime(&timeinfo);
        time(&now);

        char bodyBuf[snprintf(NULL, 0, "%ld", now)+16];
        sprintf(bodyBuf, "{\"%s\":%ld}", EmberIotStreamValues::LAST_SEEN_PATH, now);

        HTTP_LOGF("Setting last_seen in path: %s, body:\n%s\n", buf, bodyBuf);

        return HTTP_UTIL::doJsonHttpRequest(client,
                                            buf,
                                            dbUrl,
                                            HTTP_UTIL::METHOD_PATCH,
                                            bodyBuf);
    }

    bool inited;
    const char* dbUrl;
    char* path;
    EmberIotStream* stream;
    EmberIotAuth* auth;

    unsigned long lastUpdatedChannels;
    unsigned long lastHeartbeat;
    bool hasUpdateByChannel[EMBER_CHANNEL_COUNT]{};
    char updateDataByChannel[EMBER_CHANNEL_COUNT][EMBER_MAXIMUM_STRING_SIZE+1]{};
};

#endif //FIREPROP_H
