//
// Created by xav on 3/28/25.
//

#ifndef FIREPROP_H
#define FIREPROP_H

#include <EmberIotShared.h>
#include <EmberIotUtil.h>
#include <WithSecureClient.h>
#include <EmberIotAuth.h>
#include <EmberIotShared.h>
#include <EmberIotStream.h>
#include <time.h>

#define UPDATE_LAST_SEEN_INTERVAL 120000
#define EMBER_BUTTON_OFF 0
#define EMBER_BUTTON_ON 1
#define EMBER_BUTTON_PUSH 2

class EmberIot : WithSecureClient
{
public:
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
    EmberIot(const char* dbUrl,
             const char* deviceId,
             const unsigned int& boardId = 0,
             const char* username = nullptr,
             const char* password = nullptr,
             const char* webApiKey = nullptr) : dbUrl(dbUrl)
    {
        stream = nullptr;
        inited = false;
        isPaused = false;
        path = nullptr;
        auth = nullptr;
        lastUpdatedChannels = 0;
        lastHeartbeat = -UPDATE_LAST_SEEN_INTERVAL;
        snprintf(EmberIotChannels::boardId, sizeof(EmberIotChannels::boardId), "%d", boardId);
        enableHeartbeat = true;

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

    /**
     * Starts communication with Firebase, should be called after the wifi is connected succesfully.
     */
    void init()
    {
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

        if (auth != nullptr && !auth->ready())
        {
            auth->loop();
            return;
        }

        if (auth != nullptr && auth->isExpired())
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
        else if (auth != nullptr)
        {
            auth->loop();
        }

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

            unsigned int bodySize = measureJson(doc) + 2;
            char body[bodySize];
            serializeJson(doc, body, bodySize);

#ifdef ESP32
            bool result = write(body);
#elif ESP8266
            pause();
            bool result = write(body);
            resume();
#endif
            if (result)
            {
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

    /**
     * Writes a string to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, const char* value)
    {
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
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%d", value);
    }

    /**
     * Writes a double to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, double value)
    {
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%f", value);
    }

    /**
     * Writes a long to a data channel.
     * @param channel Channel number.
     * @param value Value to be written.
     */
    void channelWrite(uint8_t channel, long long value)
    {
        hasUpdateByChannel[channel] = true;
        sprintf(updateDataByChannel[channel], "%lld", value);
    }

    void pause()
    {
        isPaused = true;
        stream->stop();
        delay(100);
    }

    void resume()
    {
        delay(100);
        isPaused = false;
        stream->start();
    }

    const char* getUserUid()
    {
        return auth != nullptr ? auth->getUserUid() : nullptr;
    }

    /**
     * Enable or disable the heartbeat packet. If disabled the device will always appear as offline in the android app.
     * You can disable this to reduce transfers in the databse, if you want (you should probably disable this for board-to-board
     * communication, as it does nothing in that case).
     */
    bool enableHeartbeat;

private:
    bool write(const char* data)
    {
        if (auth != nullptr && auth->getUserUid() == nullptr)
        {
            return false;
        }

        size_t bufSize = EmberIotStreamValues::PROTOCOL_SIZE +
            strlen(dbUrl) +
            EmberIotStreamValues::AUTH_PARAM_SIZE +
            strlen(stream->getPath()) +
            (auth != nullptr ? auth->getTokenSize() : 0) + 8;

        size_t pathSize = strlen(stream->getPath());
        char finalPath[pathSize + 1];
        if (FirePropUtil::endsWith(stream->getPath(), ".json"))
        {
            strcpy(finalPath, stream->getPath());
            finalPath[pathSize - 5] = 0;
        }

        char buf[bufSize];
        sprintf_P(buf, PSTR("%S%s%s.json%S%s&print=silent"),
                EmberIotStreamValues::PROTOCOL,
                dbUrl,
                finalPath,
                EmberIotStreamValues::AUTH_PARAM,
                auth != nullptr ? auth->getToken() : "");

        HTTP_LOGF("Setting properties in path: %s, body:\n%s\n", buf, data);

        return HTTP_UTIL::doJsonHttpRequest(client,
                                            buf,
                                            dbUrl,
                                            FPSTR(HTTP_UTIL::METHOD_PATCH),
                                            data);
    }

    bool writeLastSeen()
    {
        if (auth != nullptr && auth->getUserUid() == nullptr)
        {
            return false;
        }

        size_t bufSize = EmberIotStreamValues::PROTOCOL_SIZE +
            strlen(dbUrl) +
            EmberIotStreamValues::AUTH_PARAM_SIZE +
            strlen(stream->getPath()) +
            (auth != nullptr ? auth->getTokenSize() : 0) + 8;

        size_t pathSize = strlen(stream->getPath());
        char finalPath[pathSize + EmberIotStreamValues::LAST_SEEN_PATH_SIZE + 1];
        finalPath[0] = 0;

        char* lastSlash = strrchr(stream->getPath(), '/');
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
        sprintf_P(buf, PSTR("%S%s%s.json%S%s&print=silent"),
                EmberIotStreamValues::PROTOCOL,
                dbUrl,
                finalPath,
                EmberIotStreamValues::AUTH_PARAM,
                auth != nullptr ? auth->getToken() : "");

        time_t now;
        tm timeinfo;
        getLocalTime(&timeinfo);
        time(&now);

#ifdef ESP32
        char bodyBuf[snprintf(NULL, 0, "%ld", now) + 16];
        sprintf(bodyBuf, "{\"%s\":%ld}", EmberIotStreamValues::LAST_SEEN_PATH, now);
#elif ESP8266
        char bodyBuf[snprintf(NULL, 0, "%lld", now) + 16];
        sprintf_P(bodyBuf, PSTR("{\"%S\":%lld}"), EmberIotStreamValues::LAST_SEEN_PATH, now);
#endif

        HTTP_LOGF("Setting last_seen in path: %s, body:\n%s\n", buf, bodyBuf);

        return HTTP_UTIL::doJsonHttpRequest(client,
                                            buf,
                                            dbUrl,
                                            FPSTR(HTTP_UTIL::METHOD_PATCH),
                                            bodyBuf);
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
