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

class FireProp : WithSecureClient
{
public:
    FireProp(const char* dbUrl,
             const char* deviceId,
             const char* username = nullptr,
             const char* password = nullptr,
             const char* webApiKey = nullptr) : dbUrl(dbUrl)
    {
        stream = nullptr;
        inited = false;
        path = nullptr;
        auth = nullptr;
        lastUpdatedChannels = 0;

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
    }

    void loop()
    {
        if (!inited)
        {
            return;
        }

        auth->loop();
        stream->loop();

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
                doc[buf] = updateDataByChannel[i];
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

    bool inited;
    const char* dbUrl;
    char* path;
    EmberIotStream* stream;
    EmberIotAuth* auth;

    unsigned long lastUpdatedChannels;
    bool hasUpdateByChannel[EMBER_CHANNEL_COUNT]{};
    char updateDataByChannel[EMBER_CHANNEL_COUNT][EMBER_MAXIMUM_STRING_SIZE+1]{};
};

#endif //FIREPROP_H
