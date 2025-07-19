//
// Created by xav on 3/28/25.
//

#ifndef EMBERSHARED_H
#define EMBERSHARED_H

#include <EmberChannelDefinition.h>

#ifndef EMBER_CHANNEL_COUNT
#error "Max channel count not defined, please define the channel count before importing this file like so: #define EMBER_CHANNEL_COUNT 5"
#error "It doesn't need to be the exact channel number, but it should be equal or more than the number of channels you will use. Each channel will use ~50 bytes of heap."
#endif

#define EMBER_MAX_CHANNEL_COUNT 99
#if EMBER_CHANNEL_COUNT > EMBER_MAX_CHANNEL_COUNT
#error "Only 99 channels are supported."
#endif

#ifndef EMBER_MAXIMUM_STRING_SIZE
#define EMBER_MAXIMUM_STRING_SIZE 33
#endif

#define EMBER_BOARD_ID_SIZE 8

// #define EMBER_STORAGE_USE_MEMORY
// #define EMBER_STORAGE_USE_LITTLEFS

#ifndef EMBER_STORAGE_USE_MEMORY
#ifndef EMBER_STORAGE_USE_LITTLEFS
#ifdef ESP32
#define EMBER_STORAGE_USE_MEMORY
#elif ESP8266
#define EMBER_STORAGE_USE_LITTLEFS
#endif
#endif
#endif

#include <EmberIotHttp.h>
#include <EmberIotShared.h>
#include <EmberIotUtil.h>

namespace EmberIotChannels
{
    bool started = false;
    bool firstCallbackDone = false;
    char boardId[EMBER_BOARD_ID_SIZE] = "0";
    bool reconnectedFlag = false;
    uint32_t lastValueHashes[EMBER_CHANNEL_COUNT]{};

    inline void callChannelUpdate(uint8_t c, const char* d, const char* w)
    {
        HTTP_LOGF("Found d and w for channel %d: %s, %s\n", c, d, w);
        if (w != nullptr && strcmp(w, boardId) == 0 && firstCallbackDone)
        {
            HTTP_LOGF("Event for channel %d was self-made, ignoring.\n", c);
            return;
        }

        uint32_t currentHash = FirePropUtil::fnv1aHash(d);
        bool hasChanged = currentHash != lastValueHashes[c];
        if (reconnectedFlag)
        {
            HTTP_LOGF("Checking hashes for channel %d\n", c);
            if (!hasChanged)
            {
                HTTP_LOGN("Data hashes are equal, ignoring event.");
                return;
            }
        }

        HTTP_LOGF("Calling update event for channel %d with data: %s\n", c, d);
        lastValueHashes[c] = FirePropUtil::fnv1aHash(d);
        EmberIotProp prop(d, hasChanged);
        callbacks[c](prop);
        HTTP_LOGN("Callback done.");
    }

    inline void handleBatchChannelUpdate(Stream& stream)
    {
        HTTP_LOGN("Is batch update event.");
        char channels[EMBER_CHANNEL_COUNT][8];
        const char* channelPointers[EMBER_CHANNEL_COUNT];
        for (uint8_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            snprintf(channels[i], sizeof(channels[i]), R"(CH%d":)", i);
            channelPointers[i] = channels[i];
        }

        const char* channelSearch[] = {R"("d":")", R"("w":")", "}"};

        for (uint8_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            int found = HTTP_UTIL::findFirstSkipWhitespace(stream, channelPointers, EMBER_CHANNEL_COUNT);
            if (found == -1)
            {
                HTTP_LOGN("No channels left, stopping.");
                return;
            }

            if (found >= EMBER_CHANNEL_COUNT)
            {
                HTTP_LOGN("Invalid channel number.");
                continue;
            }

            EmberIotUpdateCallback cb = callbacks[found];
            if (cb == nullptr)
            {
                HTTP_LOGF("Channel %d has no callback, skipping.\n", found);
                continue;
            }

            char w[EMBER_BOARD_ID_SIZE]{};
            char d[EMBER_MAXIMUM_STRING_SIZE]{};
            bool dataFound = false;
            for (uint8_t j = 0; j < 2; j++)
            {
                int foundProperty = HTTP_UTIL::findFirstSkipWhitespace(stream, channelSearch, 3);
                if (foundProperty == 0)
                {
                    size_t read = stream.readBytesUntil('"', d, EMBER_MAXIMUM_STRING_SIZE - 1);
                    d[read < EMBER_MAXIMUM_STRING_SIZE - 1 ? read : EMBER_MAXIMUM_STRING_SIZE - 1] = 0;
                    dataFound = true;
                }
                else if (foundProperty == 1)
                {
                    size_t read = stream.readBytesUntil('"', w, sizeof(w) - 1);
                    w[read < sizeof(w) - 1 ? read : sizeof(w) - 1] = 0;
                }
                else
                {
                    HTTP_LOGF("No data found for prop %d\n", found);
                    break;
                }
            }

            if (!dataFound)
            {
                HTTP_LOGN("Data not found for channel, skipping.");
                continue;
            }

            callChannelUpdate(found, d, w);
        }
    }

    inline void handleSingleChannelUpdate(Stream &stream)
    {
        char chStr[8];
        size_t readChStr = stream.readBytesUntil('"', chStr, sizeof(chStr) - 1);
        chStr[readChStr < sizeof(chStr) - 1 ? readChStr : sizeof(chStr) - 1] = 0;

        char* lastSlash = strrchr(chStr, '/');
        if (lastSlash != nullptr)
        {
            lastSlash[0] = 0;
        }

        HTTP_LOGF("Single channel update, chStr: C%s\n", chStr);
        int channel = -1;
        FirePropUtil::str2int_errno result = FirePropUtil::str2int(&channel, chStr + 1, 10);
        if (result != FirePropUtil::STR2INT_SUCCESS || channel < 0)
        {
            HTTP_LOGF("Error parsing channel number: %s / %d\n", chStr, result);
            return;
        }

        if (channel >= EMBER_CHANNEL_COUNT)
        {
            HTTP_LOGF("Channel %d is invalid, skipping.\n", channel);
            return;
        }

        bool foundData = HTTP_UTIL::findSkipWhitespace(stream, R"("data":")");
        if (!foundData)
        {
            HTTP_LOGF("Data not found in channel update event for channel %d\n.", channel);
            return;
        }

        char data[EMBER_MAXIMUM_STRING_SIZE];
        size_t readData = stream.readBytesUntil('"', data, EMBER_MAXIMUM_STRING_SIZE - 1);
        data[readData < EMBER_MAXIMUM_STRING_SIZE - 1 ? readData : EMBER_MAXIMUM_STRING_SIZE - 1] = 0;

        HTTP_LOGF("Single channel update data: %s\n", data);
        callChannelUpdate(channel, data, "");
    }

    inline void streamCallback(Stream& stream)
    {
        if (!started)
        {
            return;
        }

        HTTP_LOGN("Received data event.");

        if (stream.available() == 0)
        {
            HTTP_LOGN("No data to read.");
            return;
        }

        bool foundPath = HTTP_UTIL::findSkipWhitespace(stream, R"("path":"/)");
        if (!foundPath)
        {
            HTTP_LOGN("Path not found for stream data update, ignoring.");
            return;
        }

        char nextPathChar = stream.read();
        // Format: ","data":{"CH0":{"d":"0","w":"app"},"CH1":{"d":"251908","w":"0"},"CH3":{"d":"27.0","w":"app"}}}
        if (nextPathChar == '"')
        {
            handleBatchChannelUpdate(stream);
        }
        // Format: CH1/d","data":"251907"}
        else if (nextPathChar == 'C')
        {
            handleSingleChannelUpdate(stream);
        }

        firstCallbackDone = true;
        reconnectedFlag = false;
    }
}

static constexpr char EMBERIOT_STREAM_PATH[] = "/users/$uid/devices/";
static constexpr char EMBERIOT_PROP_PATH[] = "properties";

#endif
