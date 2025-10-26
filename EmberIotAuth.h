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

#ifndef FIREBASEAUTH_H
#define FIREBASEAUTH_H

#include <EmberIotHttp.h>
#include <WithSecureClient.h>
#include <LittleFS.h>

#ifndef EMBER_AUTH_MEMORY_TOKEN_SIZE
#define EMBER_AUTH_MEMORY_TOKEN_SIZE 1024
#endif

#define EMBER_AUTH_UID_SIZE 36
#define EMBER_AUTH_TOKEN_EXPIRATION 3400

namespace EmberIotAuthValues
{
    const char AUTH_PATH[] PROGMEM = "/v1/accounts:signInWithPassword?key=";
    const char AUTH_HOST[] PROGMEM = "identitytoolkit.googleapis.com";

    const char AUTH_BODY_EMAIL_1[] PROGMEM = R"({"email":")";
    const char AUTH_BODY_PASSWORD_2[] PROGMEM = R"(","password":")";
    const char AUTH_BODY_END[] PROGMEM = R"(","returnSecureToken":true})";

    const char TOKEN_PROP[] PROGMEM = R"("idToken":")";
}

class EmberIotAuth
{
public:
    EmberIotAuth(const char* username,
        const char* password,
#ifdef EMBER_STORAGE_USE_LITTLEFS
        const char* apiKey,
        const char *littleFsTempTokenLocation = "/ember-iot-temp/user-token"
#else
        const char* apiKey
#endif
    ): username(username), password(password), apiKey(apiKey)
    {
        this->apiKeySize = strlen(apiKey);
        this->userUidSet = false;
        this->tokenExpiration = 0;
        this->lastTry = 0;

#ifdef EMBER_STORAGE_USE_LITTLEFS
        size_t fileSize = strlen(littleFsTempTokenLocation);

        this->littleFsTempTokenLocation = (char*)malloc(fileSize + 1);
        strcpy(this->littleFsTempTokenLocation, littleFsTempTokenLocation);
#else
        currentToken = (char*) malloc(EMBER_AUTH_MEMORY_TOKEN_SIZE+1);
        currentToken[0] = 0;
#endif
    }

    bool isExpired() const
    {
        time_t now;
        time(&now);
        return now > tokenExpiration;
    }

    char *getUserUid()
    {
        return userUid;
    }

    void init(WithSecureClient *ch = nullptr)
    {
        FirePropUtil::initTime();

        clientHolder = ch;
        if (clientHolder == nullptr)
        {
            clientHolder = new WithSecureClient();
        }

#ifdef EMBER_STORAGE_USE_LITTLEFS
#ifdef ESP32
        if (!LittleFS.begin(false /* false: Do not format if mount failed */))
        {
            HTTP_LOGN("Failed to mount LittleFS");
            if (!LittleFS.begin(true /* true: format */))
            {
                HTTP_LOGN("Failed to format LittleFS");
            }
            else
            {
                HTTP_LOGN("LittleFS formatted successfully");
                ESP.restart();
            }
        }
#elif ESP8266
        LittleFS.begin();
#endif

        char *lastCharPtr = strrchr(littleFsTempTokenLocation, '/');
        if (lastCharPtr != nullptr)
        {
            int lastSlash = lastCharPtr - littleFsTempTokenLocation;
            char buf[lastSlash+1];
            strncpy(buf, littleFsTempTokenLocation, lastSlash);
            buf[lastSlash] = 0;
            LittleFS.mkdir(buf);
        }

        char fileLocationBuf[strlen(littleFsTempTokenLocation)+5];
        sprintf(fileLocationBuf, "%s-exp", littleFsTempTokenLocation);
        File expFile = LittleFS.open(fileLocationBuf, "r");
        if (expFile)
        {
            char expStr[expFile.size()+1];
            size_t read = expFile.readBytes(expStr, sizeof(expStr)-1);
            expStr[read < sizeof(expFile)-1 ? read : sizeof(expStr)-1] = 0;
            expFile.close();

            FirePropUtil::str2ul(&tokenExpiration, expStr, 10);
            HTTP_LOGF("Read expiration %lu from file.\n", tokenExpiration);
        }
        else
        {
            HTTP_LOGN("Auth expiration file not found.");
        }

        sprintf(fileLocationBuf, "%s-uid", littleFsTempTokenLocation);
        File uidFile = LittleFS.open(fileLocationBuf, "r");
        if (uidFile)
        {
            size_t read = uidFile.readBytes(userUid, sizeof(userUid)-1);
            userUid[read < sizeof(userUid)-1 ? read : sizeof(userUid)-1] = 0;
            uidFile.close();

            userUidSet = true;
            HTTP_LOGF("Read user uid %s from file.\n", userUid);
        }
        else
        {
            HTTP_LOGN("Auth uid file not found.");
        }
#endif
    }

    void loop()
    {
        if (!FirePropUtil::isTimeInitialized())
        {
            return;
        }

        if ((isExpired() || !this->userUidSet) && millis() - lastTry > 5000)
        {
            authenticateFirebase();
            lastTry = millis();
        }
    }

    bool ready()
    {
        return userUidSet;
    }

    void writeToken(Stream &stream)
    {
#ifdef EMBER_STORAGE_USE_LITTLEFS
        File tokenFile = LittleFS.open(littleFsTempTokenLocation, "r");
        HTTP_UTIL::printChunked(tokenFile, stream);
        tokenFile.close();
#else
        stream.print(currentToken);
#endif
    }

private:
    bool authenticateFirebase()
    {
        if (username == nullptr || password == nullptr || apiKey == nullptr)
        {
            HTTP_LOGN("Auth params are invalid, aborting.");
            return false;
        }

        EMBER_PRINT_MEM("Memory before auth request");
        WiFiClientSecure& client = clientHolder->client;

        char hostBuffer[strlen_P(EmberIotAuthValues::AUTH_HOST)+1];
        strcpy_P(hostBuffer, EmberIotAuthValues::AUTH_HOST);

        HTTP_UTIL::connectToHost(hostBuffer, client);

        HTTP_UTIL::printHttpMethod(FPSTR(HTTP_UTIL::METHOD_POST), client);
        HTTP_PRINT_BOTH(FPSTR(EmberIotAuthValues::AUTH_PATH), client);
        HTTP_PRINT_BOTH(apiKey, client);
        HTTP_UTIL::printHttpVer(client);

        HTTP_UTIL::printHost(hostBuffer, client);
        HTTP_UTIL::printContentType(client);

        size_t contentLength = strlen_P(EmberIotAuthValues::AUTH_BODY_EMAIL_1) +
            strlen_P(EmberIotAuthValues::AUTH_BODY_PASSWORD_2) +
            strlen_P(EmberIotAuthValues::AUTH_BODY_END) +
            strlen(username) +
            strlen(password);
        HTTP_UTIL::printContentLengthAndEndHeaders(contentLength, client);

        client.print(FPSTR(EmberIotAuthValues::AUTH_BODY_EMAIL_1));
        client.print(username);
        client.print(FPSTR(EmberIotAuthValues::AUTH_BODY_PASSWORD_2));
        client.print(password);
        client.print(FPSTR(EmberIotAuthValues::AUTH_BODY_END));

        EMBER_PRINT_MEM("Memory waiting auth response");

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        if (!HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGF("Error while trying to get auth token: %d\n", responseStatus);
            client.stop();
            return false;
        }

#ifdef EMBER_STORAGE_USE_LITTLEFS
        char tempLocation[strlen(littleFsTempTokenLocation)+5];
        sprintf(tempLocation, "%s-tmp", littleFsTempTokenLocation);

        File tempFile = LittleFS.open(tempLocation, "w");
        if (!tempFile)
        {
            HTTP_LOGN("Error opening temp file, cancelling.");
            client.stop();
            return false;
        }
        HTTP_UTIL::printChunked(client, tempFile);
        tempFile.close();
        client.stop();

        tempFile = LittleFS.open(tempLocation, "r");
        if (!tempFile) {
            HTTP_LOGN("Failed to reopen temp file for reading.");
            return false;
        }

        if (!HTTP_UTIL::findSkipWhitespace(tempFile, R"("idToken":")"))
        {
            HTTP_LOGN("IdToken field not found in response body.");
            tempFile.close();
            return false;
        }

        File tokenFile = LittleFS.open(littleFsTempTokenLocation, "w");
        if (!tokenFile)
        {
            HTTP_LOGN("Error opening token file, cancelling.");
            tempFile.close();
            return false;
        }

        HTTP_UTIL::printChunkedUntil(tempFile, tokenFile, R"(")");
        tokenFile.close();

        tempFile.seek(0);
        if (!HTTP_UTIL::findSkipWhitespace(tempFile, R"("localId":")"))
        {
            HTTP_LOGN("LocalId field not found in response body.");
            tempFile.close();
            return false;
        }

        size_t read = tempFile.readBytesUntil('"', userUid, EMBER_AUTH_UID_SIZE-1);
        userUid[read < EMBER_AUTH_UID_SIZE-1 ? read : EMBER_AUTH_UID_SIZE-1] = 0;

        tempFile.close();
        LittleFS.remove(tempLocation);

        time_t now;
        time(&now);
        tokenExpiration = now + EMBER_AUTH_TOKEN_EXPIRATION;

        char expFileLocation[strlen(littleFsTempTokenLocation)+5];
        sprintf(expFileLocation, "%s-exp", littleFsTempTokenLocation);
        File expFile = LittleFS.open(expFileLocation, "w");
        expFile.print(tokenExpiration);
        expFile.close();

        char uidFileLocation[strlen(littleFsTempTokenLocation)+5];
        sprintf(uidFileLocation, "%s-uid", littleFsTempTokenLocation);
        File uidFile = LittleFS.open(uidFileLocation, "w");
        uidFile.print(userUid);
        uidFile.close();
#ifdef EMBER_ENABLE_LOGGING
        tokenFile = LittleFS.open(littleFsTempTokenLocation, "r");
        expFile = LittleFS.open(expFileLocation, "r");
        uidFile = LittleFS.open(uidFileLocation, "r");
        HTTP_LOGF("Token saved succesfully: %s\n", tokenFile.readString().c_str());
        HTTP_LOGF("Uid: %s, Expiration: %lu\n", uidFile.readString().c_str(), expFile.readString().toInt());
        tokenFile.close();
        expFile.close();
        uidFile.close();
#endif

#else
        currentToken[0] = 0;
        const char *search[] = {R"("idToken":")", R"("localId":")"};

        for (uint8_t i = 0; i < 2; i++)
        {
            int found = HTTP_UTIL::findFirstSkipWhitespace(client, search, 2);
            if (found == -1)
            {
                HTTP_LOGN("Token and uid not found in stream, cancelling.");
                client.stop();
                return false;
            }

            char *store = found == 0 ? currentToken : userUid;
            size_t bufferSize = found == 0 ? EMBER_AUTH_MEMORY_TOKEN_SIZE : EMBER_AUTH_UID_SIZE;
            size_t read = client.readBytesUntil('"', store, bufferSize-1);
            store[read < bufferSize-1 ? read : bufferSize-1] = 0;
        }

        client.stop();
        HTTP_LOGF("Auth token read into memory: %s\n", currentToken);
        HTTP_LOGF("User uid read into memory: %s\n", userUid);

        time_t now;
        time(&now);
        tokenExpiration = now + 3400;
        HTTP_LOGF("Token expiration: %lu\n", tokenExpiration);
#endif

        userUidSet = true;
        HTTP_LOGN("Auth token saved succesfully.");
        return true;
    }

    const char* username;
    const char* password;
    const char* apiKey;
    uint16_t apiKeySize;

#ifdef EMBER_STORAGE_USE_LITTLEFS
    char *littleFsTempTokenLocation;
#else
    char *currentToken;
#endif

    unsigned long tokenExpiration;

    char userUid[EMBER_AUTH_UID_SIZE+1]{0};
    bool userUidSet;
    unsigned long lastTry;
    WithSecureClient *clientHolder;
};

#endif //FIREBASEAUTH_H
