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

#ifndef EMBERSHARED_H
#define EMBERSHARED_H

#include <EmberChannelDefinition.h>
#include <supertinycron/ccronexpr.h>

#ifndef EMBER_CHANNEL_COUNT
#error "Max channel count not defined, please define the channel count before importing this file like so: #define EMBER_CHANNEL_COUNT 5"
#error "It doesn't need to be the exact channel number, but it should be equal or more than the number of channels you will use. Each channel will use ~50 bytes of heap."
#endif

#define EMBER_MAX_CHANNEL_COUNT 99
#if EMBER_CHANNEL_COUNT > EMBER_MAX_CHANNEL_COUNT
#error "Only 99 channels are supported."
#endif

#ifndef EMBER_MAX_SCHEDULES
#define EMBER_MAX_SCHEDULES 20
#endif

#if EMBER_MAX_SCHEDULES > 99
#error "Only 99 schedules per device are supported."
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
    struct ScheduleJob
    {
        time_t nextExecution = -1;
        uint8_t dataChannel = 0;
        char value[EMBER_MAXIMUM_STRING_SIZE]{};
        char cron[EMBER_MAXIMUM_STRING_SIZE]{};
        char mode = 0; // d for decrement, i for increment or anything else for nothing
        int scheduleId = -1;
    };

    enum JobModes
    {
        JOB_MODE_INCREMENT = 'i',
        JOB_MODE_DECREMENT = 'd',
        JOB_MODE_SET = 's'
    };

    using ScheduleJobCallback = void(*)(ScheduleJob *job);

    bool started = false;
    bool firstCallbackDone = false;
    char boardId[EMBER_BOARD_ID_SIZE] = "0";
    bool reconnectedFlag = false;
    char lastValues[EMBER_CHANNEL_COUNT][EMBER_MAXIMUM_STRING_SIZE]{};

    ScheduleJob *jobs[EMBER_MAX_SCHEDULES]{nullptr};
    ScheduleJobCallback jobCallbacks[EMBER_MAX_SCHEDULES]{nullptr};

    inline void callChannelUpdate(uint8_t c, const char* d, const char* w)
    {
        HTTP_LOGF("Found d and w for channel %d: %s, %s\n", c, d, w);
        if (w != nullptr && strcmp(w, boardId) == 0 && firstCallbackDone)
        {
            HTTP_LOGF("Event for channel %d was self-made, ignoring.\n", c);
            return;
        }

        bool hasChanged = strcmp(d, lastValues[c]);
        if (reconnectedFlag)
        {
            HTTP_LOGF("Checking hashes for channel %d\n", c);
            if (!hasChanged)
            {
                HTTP_LOGN("New data is equal to last data, ignoring event.");
                return;
            }
        }

        HTTP_LOGF("Calling update event for channel %d with data: %s\n", c, d);
        snprintf(lastValues[c], EMBER_MAXIMUM_STRING_SIZE, "%s", d);
        EmberIotProp prop(d, hasChanged);
        callbacks[c](prop);
        HTTP_LOGN("Callback done.");
    }

    inline void handleSingleChannelUpdate(Stream &stream)
    {
        char chStr[8]{};
        stream.readBytesUntil('"', chStr, sizeof(chStr) - 1);

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

        char data[EMBER_MAXIMUM_STRING_SIZE]{};
        stream.readBytesUntil('"', data, EMBER_MAXIMUM_STRING_SIZE - 1);

        HTTP_LOGF("Single channel update data: %s\n", data);
        callChannelUpdate(channel, data, "");
    }

    inline void updateScheduleNextExecution(const int id)
    {
        ScheduleJob *job = jobs[id];
        if (job == nullptr || strlen(job->cron) == 0 || job->dataChannel < 0 || job->scheduleId < 0)
        {
            return;
        }

        job->nextExecution = -1;
        EMBER_DEBUGF("Parsing cron expression: %s\n", job->cron);

        cron_expr expr{};
        const char *err = nullptr;
        cron_parse_expr(job->cron, &expr, &err);

        if (err != nullptr)
        {
            EMBER_DEBUGF("Error parsing cron from schedule %d: %s\n", id, err);
            return;
        }

        job->nextExecution = cron_next(&expr, time(nullptr));
        EMBER_DEBUGF("Parsed successfully, next execution: %ld\n", job->nextExecution);
    }

    inline void handleUpdateSchedule(Stream &stream, int id = -1)
    {
        if (id == -1)
        {
            char idStr[8]{};
            stream.readBytesUntil('"', idStr, sizeof(idStr) - 1);

            FirePropUtil::str2int_errno result = FirePropUtil::str2int(&id, idStr + 1, 10);
            if (result != FirePropUtil::STR2INT_SUCCESS || id < 0)
            {
                HTTP_LOGF("Error parsing schedule id number: %s / %d\n", idStr, result);
                return;
            }
        }

        EMBER_DEBUGF("Starting update schedule with id %d\n", id);
        if (id >= EMBER_MAX_SCHEDULES)
        {
            HTTP_LOGN("Schedule updated is higher than maximum allowed, increase the schedule limit.");
            return;
        }

        if (jobs[id] == nullptr)
        {
            jobs[id] = new ScheduleJob();
        }

        ScheduleJob *job = jobs[id];
        job->cron[0] = 0;
        job->value[0] = 0;
        job->mode = 0;
        job->dataChannel = -1;
        job->scheduleId = id;

        const char *search[] = {"\"cn\":\"", "\"vl\":\"", "\"md\":\"", "\"cron\":\"", "}"};
        size_t searchLen = 4;
        for (int i = 0; i < searchLen; i++)
        {
            int found = HTTP_UTIL::findFirstSkipWhitespace(stream, search, searchLen, true);
            if (found == 0 || found == 1 || found == 3)
            {
                char buf[EMBER_MAXIMUM_STRING_SIZE]{};
                stream.readBytesUntil('"', buf, EMBER_MAXIMUM_STRING_SIZE-1);

                if (found == 0)
                {
                    int out = -1;
                    FirePropUtil::str2int_errno ret = FirePropUtil::str2int(&out, buf, 10);
                    if (ret != FirePropUtil::STR2INT_SUCCESS)
                    {
                        HTTP_LOGF("Error converting string to int: %d\n", ret);
                        continue;
                    }

                    EMBER_DEBUGF("Channel for schedule: %d\n", out);
                    job->dataChannel = out;
                }
                else if (found == 1)
                {
                    EMBER_DEBUGF("Value for schedule: %s\n", buf);
                    snprintf(job->value, EMBER_MAXIMUM_STRING_SIZE, "%s", buf);
                }
                else
                {
                    EMBER_DEBUGF("Cron for schedule: %s\n", buf);
                    snprintf(job->cron, EMBER_MAXIMUM_STRING_SIZE, "%s", buf);
                }
            }
            else if (found == 2)
            {
                job->mode = stream.read();
                EMBER_DEBUGF("Mode for schedule: %c\n", job->mode);
            }
            else
            {
                break;
            }
        }

        if (strlen(job->cron) == 0 || job->dataChannel < 0 || job->scheduleId < 0)
        {
            EMBER_DEBUGN("Invalid schedule data, removing scheduling.");
            delete jobs[id];
            jobs[id] = nullptr;
            return;
        }

        updateScheduleNextExecution(id);
        EMBER_DEBUGN("Schedule updated.");
    }

    inline void handleBatchPropertyUpdate(Stream& stream)
    {
        HTTP_LOGN("Is batch update event.");
        const char* channelSearch[] = {R"("d":")", R"("w":")", "}"};
        size_t channelSearchSize = 3;
        const char* properties[] = {"\"CH", "\"SC"};
        size_t propertiesSize = 2;

        for (size_t i = 0; i < EMBER_CHANNEL_COUNT+EMBER_MAX_SCHEDULES; i++)
        {
            int found = HTTP_UTIL::findFirstSkipWhitespace(stream, properties, propertiesSize);
            if (found < 0)
            {
                EMBER_DEBUGN("No more properties found.");
                return;
            }

            char numBuf[8]{};
            stream.readBytesUntil('"', numBuf, sizeof(numBuf)-1);
            size_t numBufLen = strlen(numBuf);
            if (numBufLen > 1)
            {
                numBuf[numBufLen-1] = 0;
            }

            if (!HTTP_UTIL::findSkipWhitespace(stream, ":{"))
            {
                EMBER_DEBUGN("Didn't find next colon and object opening bracket.");
                continue;
            }

            int id = -1;
            FirePropUtil::str2int_errno idRet = FirePropUtil::str2int(&id, numBuf, 10);
            if (idRet != FirePropUtil::STR2INT_SUCCESS)
            {
                EMBER_DEBUGF("Error parsing num for %s: %d\n", numBuf, idRet);
                continue;
            }

            EMBER_DEBUGF("Found prop of type %d with id: %d\n", found, id);

            if (found == 0)
            {
                if (id >= EMBER_CHANNEL_COUNT || id < 0)
                {
                    EMBER_DEBUGN("Invalid channel number.");
                    continue;
                }

                if (callbacks[id] == nullptr)
                {
                    EMBER_DEBUGF("Channel %d has no callback, skipping.\n", id);
                    continue;
                }

                char w[EMBER_BOARD_ID_SIZE]{};
                char d[EMBER_MAXIMUM_STRING_SIZE]{};
                bool dataFound = false;
                for (uint8_t j = 0; j < channelSearchSize; j++)
                {
                    int foundProperty = HTTP_UTIL::findFirstSkipWhitespace(stream, channelSearch, channelSearchSize);
                    if (foundProperty == 0)
                    {
                        size_t read = stream.readBytesUntil('"', d, EMBER_MAXIMUM_STRING_SIZE - 1);
                        d[read < EMBER_MAXIMUM_STRING_SIZE - 1 ? read : EMBER_MAXIMUM_STRING_SIZE - 1] = 0;
                        dataFound = true;
                    }
                    else if (foundProperty == 1)
                    {
                        stream.readBytesUntil('"', w, sizeof(w) - 1);
                    }
                    else
                    {
                        EMBER_DEBUGF("No data found for prop %d\n", found);
                        break;
                    }
                }

                if (!dataFound)
                {
                    EMBER_DEBUGN("Data not found for channel, skipping.");
                    continue;
                }

                callChannelUpdate(id, d, w);
            }
            else if (found == 1)
            {
                handleUpdateSchedule(stream, id);
            }
        }
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
        // Format: ","data":{"CH0":{"d":"0","w":"app"},"CH1":{"d":"251908","w":"0"},"CH3":{"d":"27.0","w":"app"},"SCH0":{"ch":0,"md":"s","vl":"value"}}}
        if (nextPathChar == '"')
        {
            handleBatchPropertyUpdate(stream);
        }
        // Format: CH1/d","data":"251907"}
        else if (nextPathChar == 'C')
        {
            handleSingleChannelUpdate(stream);
        }
        // Format: SC0","data":{"md":"s","ch":"0","vl":"value"}}
        else if (nextPathChar == 'S')
        {
            handleUpdateSchedule(stream);
        }

        firstCallbackDone = true;
        reconnectedFlag = false;
    }

    inline bool setScheduleCallback(int scheduleId, ScheduleJobCallback fn)
    {
        if (scheduleId < 0)
        {
            return false;
        }

        if (scheduleId >= EMBER_MAX_SCHEDULES)
        {
            return false;
        }

        jobCallbacks[scheduleId] = fn;
        return true;
    }
}

static constexpr char EMBERIOT_STREAM_PATH[] = "/users/$uid/devices/";
static constexpr char EMBERIOT_PROP_PATH[] = "properties";

#endif
