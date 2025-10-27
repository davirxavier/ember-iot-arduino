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

#ifndef FIREPROP_H
#define FIREPROP_H

#include <EmberIotShared.h>
#include <EmberIotUtil.h>
#include <WithSecureClient.h>
#include <EmberIotAuth.h>
#include <EmberIotShared.h>
#include <EmberIotShared.h>
#include <EmberIotStream.h>
#include <time.h>

#define UPDATE_LAST_SEEN_INTERVAL 120000
#define EMBER_BUTTON_OFF 0
#define EMBER_BUTTON_ON 1
#define EMBER_BUTTON_PUSH 2

/**
 * @param dbUrl Realtime database URL, without protocol and slashes at the end. Example value: my-rtdb.firebaseio.com
 * @param deviceId Device id string. Should be the device id copied from the android app (by long-pressing on a device)
 * or any string if you want to use board-to-board communication only.
 * @param boardId Per device unique identifier for this specific board. You need to use separate numbers for board-to-board
 * communication, so the library can identify who emitted what data. Use 0 or any positive number if only using with the android app.
 * @param username Firebase authentication username. The username for the user you created in Firebase Authentication (not the username used to sign in to firebase).
 * @param password Firebase authentication password. The password for the user you created in Firebase Authentication (not the username used to sign in to firebase).
 * @param webApiKey Api key for your Firebase project, found under project settings.
 */
class EmberIot : WithSecureClient
{
public:
    EmberIot(const char* dbUrl,
             const char* deviceId,
             const char* username,
             const char* password,
             const char* webApiKey,
             const unsigned int& boardId = 0) : dbUrl(dbUrl)
    {
        stream = nullptr;
        inited = false;
        isPaused = false;
        path = nullptr;
        lastUpdatedChannels = 0;
        lastHeartbeat = -UPDATE_LAST_SEEN_INTERVAL;
        snprintf(EmberIotChannels::boardId, sizeof(EmberIotChannels::boardId), "%d", boardId);
        enableHeartbeat = true;

        auth = new EmberIotAuth(username, password, webApiKey);

        const size_t pathSize = strlen(EMBERIOT_STREAM_PATH) + strlen(deviceId) + strlen(EMBERIOT_PROP_PATH) + 2;
        path = new char[pathSize]{};
        snprintf(path, pathSize, "%s%s/%s", EMBERIOT_STREAM_PATH, deviceId, EMBERIOT_PROP_PATH);
        stream = new EmberIotStream(auth, dbUrl, path);
        stream->setCallback(EmberIotChannels::streamCallback);

        for (size_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            hasUpdateByChannel[i] = false;
            updateDataByChannel[i][0] = 0;
        }
    }

    /**
     * Starts communication with Firebase, should be called after the wifi is connected succesfully.
     */
    void init()
    {
        auth->init(this);
        stream->start();
        inited = true;
        EmberIotChannels::started = true;
        isPaused = false;
        FirePropUtil::initTime();
    }

    /**
     * Manages the connection with Firebase, should be called every loop.
     */
    void loop()
    {
        if (!inited)
        {
            return;
        }

        if (auth == nullptr || stream == nullptr)
        {
            return;
        }

        if (!auth->ready())
        {
            auth->loop();
            return;
        }

        if (auth->isExpired())
        {
#ifdef ESP32
            auth->loop();
#elif ESP8266
            pause();
            auth->loop();
            resume();
#endif
            return;
        }

        auth->loop();

        if (!stream->isConnected())
        {
            EmberIotChannels::reconnectedFlag = true;
        }

        stream->loop();

        if (enableHeartbeat && millis() - lastHeartbeat > UPDATE_LAST_SEEN_INTERVAL)
        {
#ifdef ESP32
            bool written = writeLastSeen();
#elif ESP8266
            pause();
            bool written = writeLastSeen();
            resume();
#endif

            if (written)
            {
                lastHeartbeat = millis();
            }
            else
            {
                HTTP_LOGN("Write heartbeat failed, trying again in one second.");
                lastHeartbeat = millis() - UPDATE_LAST_SEEN_INTERVAL + 2000;
            }
        }

        if (millis() - lastUpdatedChannels < 500)
        {
            return;
        }

        runPendingSchedules();

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
#ifdef ESP32
            bool result = updateChannels();
#elif ESP8266
            pause();
            bool result = updateChannels();
            resume();
#endif
            if (result)
            {
                for (size_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
                {
                    if (hasUpdateByChannel[i])
                    {
                        snprintf(EmberIotChannels::lastValues[i], EMBER_MAXIMUM_STRING_SIZE, "%s", updateDataByChannel[i]);
                    }
                }

                for (bool& i : hasUpdateByChannel)
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

    EmberIotProp getChannelLastValue(const int channel)
    {
        return EmberIotProp(EmberIotChannels::lastValues[channel], false);
    }

    /**
     * Sets a callback to be executed when a scheduled action runs.
     * @param scheduleId schedule id.
     * @param callback function to be executed.
     */
    void setScheduleCallback(int scheduleId, EmberIotChannels::ScheduleJobCallback callback)
    {
        EmberIotChannels::setScheduleCallback(scheduleId, callback);
    }

    /**
     * Writes a string to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, const char* value)
    {
        if (!checkChannelChanged(updateDataByChannel[channel], value))
        {
            return;
        }

        hasUpdateByChannel[channel] = true;
        strncpy(updateDataByChannel[channel], value, EMBER_MAXIMUM_STRING_SIZE);
        updateDataByChannel[channel][EMBER_MAXIMUM_STRING_SIZE] = 0;
    }

    /**
     * Writes an int to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, int value)
    {
        char newValStr[EMBER_MAXIMUM_STRING_SIZE];
        sprintf(newValStr, "%d", value);
        channelWrite(channel, newValStr);
    }

    /**
     * Writes a double to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, double value)
    {
        char newValStr[EMBER_MAXIMUM_STRING_SIZE];
        sprintf(newValStr, "%f", value);
        channelWrite(channel, newValStr);
    }

    /**
     * Writes a long to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, long long value)
    {
        char newValStr[EMBER_MAXIMUM_STRING_SIZE];
        sprintf(newValStr, "%lld", value);
        channelWrite(channel, newValStr);
    }

    void pause()
    {
        isPaused = true;
        stream->stop();
        delay(50);
    }

    void resume()
    {
        delay(50);
        isPaused = false;
        stream->start();
    }

    const char* getUserUid()
    {
        return auth != nullptr && auth->ready() ? auth->getUserUid() : nullptr;
    }

    /**
     * Enable or disable the heartbeat packet. If disabled the device will always appear as offline in the android app.
     * You can disable this to reduce transfers in the databse, if you want (you should probably disable this for board-to-board
     * communication, as it does nothing in that case).
     */
    bool enableHeartbeat;

    WiFiClientSecure *getWifiClient()
    {
        return &client;
    }

private:
    void runPendingSchedules()
    {
        time_t now = time(nullptr);

        for (size_t i = 0; i < EMBER_MAX_SCHEDULES; i++)
        {
            EmberIotChannels::ScheduleJob *job = EmberIotChannels::jobs[i];
            if (job == nullptr || job->nextExecution < 0 || job->nextExecution > now || job->dataChannel < 0 || job->dataChannel >= EMBER_CHANNEL_COUNT)
            {
                continue;
            }

            char writtenData[EMBER_MAXIMUM_STRING_SIZE]{};
            switch (tolower(job->mode))
            {
                case EmberIotChannels::JOB_MODE_DECREMENT:
                case EmberIotChannels::JOB_MODE_INCREMENT:
                    {
                        double out;
                        FirePropUtil::str2int_errno ret = FirePropUtil::str2double(&out, job->value);
                        if (ret != FirePropUtil::STR2INT_SUCCESS)
                        {
                            EMBER_DEBUGF("Error parsing job value/str: %d/%s\n", ret, job->value);
                            break;
                        }

                        double dataChannelValue;
                        FirePropUtil::str2int_errno retChannel = FirePropUtil::str2double(&dataChannelValue, EmberIotChannels::lastValues[job->dataChannel]);
                        if (retChannel == FirePropUtil::STR2INT_INCONVERTIBLE)
                        {
                            EMBER_DEBUGN("Current data in channel is inconvertible to a number, setting to schedule value.");
                            channelWrite(job->dataChannel, job->value);
                            snprintf(writtenData, sizeof(writtenData), "%s", job->value);
                            break;
                        }
                        else if (retChannel != FirePropUtil::STR2INT_SUCCESS)
                        {
                            EMBER_DEBUGF("Error parsing data channel value/str: %d/%s\n", ret, EmberIotChannels::lastValues[job->dataChannel]);
                            break;
                        }

                        double newValue = job->mode == EmberIotChannels::JOB_MODE_INCREMENT ? dataChannelValue + out : dataChannelValue - out;
                        channelWrite(job->dataChannel, newValue);
                        snprintf(writtenData, sizeof(writtenData), "%lf", newValue);
                        break;
                    }
            default:
                {
                    channelWrite(job->dataChannel, job->value);
                }
            }

            EmberIotChannels::updateScheduleNextExecution(job->scheduleId);

            if (EmberIotChannels::jobCallbacks[job->scheduleId] != nullptr)
            {
                EmberIotChannels::jobCallbacks[job->scheduleId](job);
            }

            if (strlen(writtenData) > 0)
            {
                EmberIotChannels::callChannelUpdate(job->dataChannel, writtenData, "sched");
            }
        }
    }

    bool checkChannelChanged(const char *lastVal, const char *newVal)
    {
        if (lastVal != nullptr && newVal == nullptr)
        {
            return true;
        }

        if (lastVal == nullptr && newVal != nullptr)
        {
            return true;
        }

        if (lastVal == nullptr && newVal == nullptr)
        {
            return false;
        }

        return strcmp(lastVal, newVal) != 0;
    }

    bool updateChannels()
    {
        if (auth != nullptr && auth->getUserUid() == nullptr)
        {
            HTTP_LOGN("Auth is defined but uid is not yet defined, aborting.");
            return false;
        }

        EMBER_PRINT_MEM("Memory before channel update");
        HTTP_LOGN("Sending channel update.");

        if (!HTTP_UTIL::connectToHost(dbUrl, client))
        {
            return false;
        }

        size_t toUpdateCount = 0;
        size_t toUpdate[EMBER_CHANNEL_COUNT];
        for (size_t i = 0; i < EMBER_CHANNEL_COUNT; i++)
        {
            if (hasUpdateByChannel[i])
            {
                toUpdate[toUpdateCount] = i;
                toUpdateCount++;
            }
        }

        HTTP_UTIL::printHttpMethod(FPSTR(HTTP_UTIL::METHOD_PATCH), client);

        HTTP_PRINT_BOTH_2(stream->getPath());
        if (!FirePropUtil::endsWith(stream->getPath(), ".json"))
        {
            HTTP_PRINT_BOTH_2(F(".json"));
        }

        if (auth != nullptr)
        {
            HTTP_PRINT_BOTH_2(EmberIotStreamValues::AUTH_PARAM);
            auth->writeToken(client);
            HTTP_PRINT_BOTH_2(F("&print=silent"));
        }
        else
        {
            HTTP_PRINT_BOTH_2(F("?print=silent"));
        }
        HTTP_UTIL::printHttpVer(client);

        HTTP_UTIL::printHost(dbUrl, client);
        HTTP_UTIL::printContentType(client);

        // 2 = {} (body brackets)
        // -1 = , (last one has no comma at end)
        size_t contentLength = 1;
        for (size_t i = 0; i < toUpdateCount; i++)
        {
            char *data = updateDataByChannel[toUpdate[i]];

            // 3 = "CH
            // 8 = ":{"d":"
            // 8 = ", "w":"
            // 2 = "}
            // 1 = ,
            // "CHx":{"d":"", "w":""},
            contentLength += 22 + snprintf(nullptr, 0, "%d", toUpdate[i]) + strlen(data) + strlen(EmberIotChannels::boardId);
        }
        HTTP_UTIL::printContentLengthAndEndHeaders(contentLength, client);

        HTTP_PRINT_BOTH_2("{");
        for (size_t i = 0; i < toUpdateCount; i++)
        {
            char *data = updateDataByChannel[toUpdate[i]];
            if (data == nullptr)
            {
                HTTP_LOGF("Error reading channels to update, index %d is nullptr.\n", i);
                client.stop();
                return false;
            }

            // {"CHx":{"d":"data","w":"boardId"}}
            HTTP_PRINT_BOTH_2(R"("CH)");
            HTTP_PRINT_BOTH_2(toUpdate[i]);
            HTTP_PRINT_BOTH_2(R"(":{"d":")");
            HTTP_PRINT_BOTH_2(data);
            HTTP_PRINT_BOTH_2(R"(", "w":")");
            HTTP_PRINT_BOTH_2(EmberIotChannels::boardId);
            HTTP_PRINT_BOTH_2(R"("})");

            if (i < toUpdateCount-1)
            {
                HTTP_PRINT_BOTH_2(",");
            }
        }
        HTTP_PRINT_BOTH_2("}");
        EMBER_DEBUGN();

        EMBER_PRINT_MEM("Memory waiting channel update response");

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        client.stop();
        if (!HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGF("Error while setting property: %d\n", responseStatus);
            return false;
        }
        return true;
    }

    bool writeLastSeen()
    {
        if (auth != nullptr && auth->getUserUid() == nullptr)
        {
            HTTP_LOGN("Auth is defined but uid is not yet defined, aborting.");
            return false;
        }

        EMBER_PRINT_MEM("Memory before last seen update");

        if (!HTTP_UTIL::connectToHost(dbUrl, client))
        {
            HTTP_LOGN("Couldn't connect.");
            return false;
        }

        char *pathLastSlash = strrchr(stream->getPath(), '/');
        size_t pathLastSlashIndex = strlen(stream->getPath());
        if (pathLastSlash != nullptr)
        {
            pathLastSlashIndex = pathLastSlash - stream->getPath();
        }

        time_t now;
        time(&now);

#ifdef ESP32
        HTTP_LOGF("Setting last_seen to %ld.\n", now);
#elif ESP8266
        HTTP_LOGF("Setting last_seen to %lld.\n", now);
#endif

        HTTP_UTIL::printHttpMethod(FPSTR(HTTP_UTIL::METHOD_PATCH), client);
        client.write((uint8_t*) stream->getPath(), pathLastSlashIndex);
        client.print(".json");

        if (auth != nullptr)
        {
            client.print("?auth=");
            auth->writeToken(client);
            client.print(F("&print=silent"));
        }
        else
        {
            client.print(F("?print=silent"));
        }
        HTTP_UTIL::printHttpVer(client);

        HTTP_UTIL::printHost(dbUrl, client);
        HTTP_UTIL::printContentType(client);

#ifdef ESP32
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        size_t length = snprintf(NULL, 0, "%lld", now);
#else
        size_t length = snprintf(NULL, 0, "%ld", now);
#endif
#elif ESP8266
        size_t length = snprintf(NULL, 0, "%lld", now);
#endif

        HTTP_UTIL::printContentLengthAndEndHeaders(strlen(EmberIotStreamValues::LAST_SEEN_BODY) + length + 1, client);

        client.print(FPSTR(EmberIotStreamValues::LAST_SEEN_BODY));
        client.print(now);
        client.print("}");

        EMBER_PRINT_MEM("Memory waiting last seen update response");

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        client.stop();
        if (!HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGF("Error while trying to set last seen: %d\n", responseStatus);
            return false;
        }

        return true;
    }

    bool inited;
    const char* dbUrl;
    char* path;
    EmberIotStream* stream;
    EmberIotAuth* auth;
    bool isPaused;

    unsigned long lastUpdatedChannels;
    unsigned long lastHeartbeat;
    bool hasUpdateByChannel[EMBER_CHANNEL_COUNT]{};
    char updateDataByChannel[EMBER_CHANNEL_COUNT][EMBER_MAXIMUM_STRING_SIZE + 1]{};
};

#endif //FIREPROP_H
