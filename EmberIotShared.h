//
// Created by xav on 3/28/25.
//

#ifndef EMBERSHARED_H
#define EMBERSHARED_H

#ifndef EMBER_CHANNEL_COUNT
#error "Max channel count not defined, please define the channel count before importing this file like so: #define EMBER_CHANNEL_COUNT 5"
#error "It doesn't need to be the exact channel number, but it should be equal or more than the number of channels you will use."
#endif

#ifndef EMBER_MAXIMUM_STRING_SIZE
#define EMBER_MAXIMUM_STRING_SIZE 32
#endif

#define EMBER_CHANNEL_CB(channel) \
    void emberIotChannelUpdateCH ## channel (const EmberIotProp &prop); \
    struct AutoRegisterCH ## channel { \
    AutoRegisterCH ## channel() { EmberIotChannels::addCallback(channel, emberIotChannelUpdateCH ## channel); } \
    } autoRegisterCH ## channel; \
    void emberIotChannelUpdateCH ## channel (const EmberIotProp &prop)

#include <EmberIotHttp.h>

class EmberIotProp
{
public:
    explicit EmberIotProp(const char* data) : data(data)
    {
    }

    int toInt() const
    {
        return data != nullptr ? atoi(data) : 0;
    }

    long toLong() const
    {
        return data != nullptr ? atol(data) : 0l;
    }

    long long toLongLong() const
    {
        return data != nullptr ? atoll(data) : 0ll;
    }

    double toDouble() const
    {
        return data != nullptr ? atof(data) : 0.0;
    }

    const char* toString() const
    {
        return this->data;
    }

private:
    const char* data;
};

typedef void (*EmberIotUpdateCallback)(const EmberIotProp& p);

namespace EmberIotChannels
{
    bool started = false;
    EmberIotUpdateCallback callbacks[EMBER_CHANNEL_COUNT]{};
    bool firstCallbackDone = false;
    char boardId[8] = "0";

    inline void streamCallback(const char* data)
    {
        if (!started)
        {
            return;
        }

        HTTP_LOGF("Received data event: %s\n", data);

        JsonDocument doc;
        deserializeJson(doc, data);

        for (size_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            const EmberIotUpdateCallback cb = callbacks[i];
            if (cb == nullptr)
            {
                continue;
            }

            char name[8];
            sprintf(name, "/CH%d", i);

            const char* path = doc["path"];
            if (strcmp("/", path) == 0)
            {
                sprintf(name, "CH%d", i);

                JsonDocument dataDoc = doc["data"];

                if (!dataDoc[name].is<JsonVariant>())
                {
                    continue;
                }

                if (dataDoc[name]["w"].is<JsonVariant>() && firstCallbackDone)
                {
                    const char *who = dataDoc[name]["w"].as<const char*>();
                    if (strcmp(who, boardId) == 0)
                    {
                        HTTP_LOGN("Event was self-made, ignoring.");
                        continue;
                    }
                }

                EmberIotProp prop(dataDoc[name]["d"].as<const char*>());
                cb(prop);
            }
        }

        firstCallbackDone = true;
    }

    inline void addCallback(uint8_t channel, EmberIotUpdateCallback cb)
    {
        callbacks[channel] = cb;
    }
}

static constexpr char EMBERIOT_STREAM_PATH[] = "/users/$uid/devices/";
static constexpr char EMBERIOT_PROP_PATH[] = "properties";

#endif
