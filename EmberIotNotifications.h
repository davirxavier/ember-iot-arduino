#ifndef FIREPROP_NOTIF_H
#define FIREPROP_NOTIF_H

#include <time.h>
#include <EmberIot.h>
#include <EmberIotHttp.h>
#include <EmberIotCryptUtil.h>

#ifdef EMBER_STORAGE_USE_LITTLEFS
#include <LittleFS.h>
#endif

#ifndef EMBER_NOTIFICATION_QUEUE_SIZE
#define EMBER_NOTIFICATION_QUEUE_SIZE 5
#endif

#ifndef EMBER_NOTIFICATION_MAX_TITLE_SIZE
#define EMBER_NOTIFICATION_MAX_TITLE_SIZE 32
#endif

#ifndef EMBER_NOTIFICATION_MAX_TEXT_SIZE
#define EMBER_NOTIFICATION_MAX_TEXT_SIZE 100
#endif

namespace EmberIotNotificationValues
{
    const char AUTH_PATH[] PROGMEM = "/token";
    const char AUTH_HOST[] PROGMEM = "oauth2.googleapis.com";

    const char SEND_NOTIF_PATH[] PROGMEM = "/v1/projects/ember-iot/messages:send";
    const char SEND_NOTIF_HOST[] PROGMEM = "fcm.googleapis.com";

    const char SEND_NOTIF_BODY_TOPIC_1[] PROGMEM = R"({"message":{"topic":")";
    const char SEND_NOTIF_BODY_TITLE_2[] PROGMEM = R"(","data":{"title":")";
    const char SEND_NOTIF_BODY_TEXT_3[] PROGMEM = R"(","body":")";
    const char SEND_NOTIF_BODY_DEVID_4[] PROGMEM = R"(","deviceId":")";
    const char SEND_NOTIF_BODY_SOUND_5[] PROGMEM = R"(","soundId":")";
    const char SEND_NOTIF_BODY_SOUND_DURATION_6[] PROGMEM = R"(","soundDurationSeconds":")";
    const char SEND_NOTIF_BODY_SOUND_LOOP_7[] PROGMEM = R"(","soundLoop":")";
    const char SEND_NOTIF_BODY_END[] PROGMEM = R"("}}})";

    const char GRANT_STR[] PROGMEM = R"({"grant_type": "urn:ietf:params:oauth:grant-type:jwt-bearer","assertion":")";
    const uint16_t GRANT_STR_SIZE = strlen_P(GRANT_STR);

    const char JWT_HEADER_B64[] PROGMEM = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.";
    const uint16_t JWT_HEADER_B64_SIZE = strlen_P(JWT_HEADER_B64);
}

struct EmberIotNotification
{
    char title[EMBER_NOTIFICATION_MAX_TITLE_SIZE];
    char text[EMBER_NOTIFICATION_MAX_TEXT_SIZE];
    int soundId = 0;
    uint16_t soundDurationSeconds = 60;
    bool soundLoop = false;
};

enum EmberSendNotificationStatus
{
    EMBER_NOTIF_QUEUED,
    EMBER_NOTIF_QUEUE_FULL,
    EMBER_NOTIF_TITLE_TOO_BIG,
    EMBER_NOTIF_TEXT_TOO_BIG
};

class FCMEmberNotifications
{
public:
    /**
     * New notifications instance.
     * @param gcmAccountEmail Google service account e-mail. Should be a flash string.
     *      DON'T USE THE FIREBASE MAIN SERVICE ACCOUNT, IT'S A VERY BIG SECURITY RISK, AS IT HAS EVERY POSSIBLE PERMISSION.
     * @param gcmAccountPrivateKey Google service account primary key. Should be a flash string.
     * @param littleFsTempTokenLocation Where to persist the generated oauth token in storage, if using LittleFS storage. Default is "/ember-iot-temp/notif-token".
     */
    FCMEmberNotifications(
        const char *gcmAccountEmail,
#ifdef EMBER_STORAGE_USE_LITTLEFS
        const __FlashStringHelper *gcmAccountPrivateKey,
        const char *littleFsTempTokenLocation = "/ember-iot-temp/notif-token"
#else
        const char *gcmAccountPrivateKey
#endif
    ):
        gcmAccountEmail(gcmAccountEmail),
        gcmAccountPrivateKey(gcmAccountPrivateKey),
        tokenExpiration(0),
        lastExpirationCheck(0),
        lastSentNotifications(0),
        forceRenew(false),
        uidInit(false),
        userUid{},
        emberInstance(nullptr),
        clientPtr(nullptr),
        currentNotification(0),
        notificationQueue{},
        initCalled(false),
        deviceId(nullptr),
        deviceName(nullptr)
    {
#ifdef EMBER_STORAGE_USE_LITTLEFS
        size_t fileSize = strlen(littleFsTempTokenLocation);
        this->littleFsTempTokenLocation = (char*) malloc(fileSize + 1);
        strcpy(this->littleFsTempTokenLocation, littleFsTempTokenLocation);
#endif
    }

    /**
     * Initializes this instance. Always use this init function if using other EmberIoT features.
     * @param emberInstance
     */
    void init(EmberIot *emberInstance)
    {
        doInit(nullptr, emberInstance);
    }

    /**
     * Initializes this instance with only an userUid. Use this init function if you are not using any other EmberIoT features.
     * @param userUid User to send notifications to, can be any Firebase auth user from your project.
     */
    void init(const char *userUid)
    {
        doInit(userUid, nullptr);
    }

    /**
     * Sets additional info for this device to send in the notification.
     * @param deviceId Id string for this device, if this is specified, the user will be redirected directly to the
     * device screen when clicking in the notification (as well as for other things on the app). Can be null.
     * @param deviceName The name that will appear on the notification text, to identify this device to the user.
     */
    void setAdditionalDeviceInfo(const char *deviceId, const char *deviceName)
    {
        if (deviceId != nullptr)
        {
            if (this->deviceId != nullptr)
            {
                delete this->deviceId;
            }

            this->deviceId = new char[strlen(deviceId)+1];
            strcpy(this->deviceId, deviceId);
        }

        if (deviceName != nullptr)
        {
            if (this->deviceName != nullptr)
            {
                delete this->deviceName;
            }

            this->deviceName = new char[strlen(deviceName)+1];
            strcpy(this->deviceName, deviceName);
        }
    }

    /**
     * Should be called every loop.
     */
    void loop()
    {
        if (!initCalled)
        {
            return;
        }

        if (!FirePropUtil::isTimeInitialized() || clientPtr == nullptr)
        {
            return;
        }

        if (emberInstance != nullptr && !uidInit && emberInstance->getUserUid() != nullptr)
        {
            uidInit = true;
            strcpy(userUid, emberInstance->getUserUid());
        }

        if (!uidInit)
        {
            return;
        }

        if (millis() - lastExpirationCheck > 2000)
        {
            time_t now;
            time(&now);

            if (now > tokenExpiration || forceRenew)
            {
                HTTP_LOGN("Notification token expired, renewing.");

#ifdef ESP32
                renewToken();
#elif ESP8266
                emberInstance->pause();
                renewToken();
                emberInstance->resume();
#endif

                forceRenew = false;
            }

            lastExpirationCheck = millis();
        }
        else  if (millis() - lastSentNotifications > 150)
        {
#ifdef ESP32
            sendPending();
#elif ESP8266
            emberInstance->pause();
            sendPending();
            emberInstance->resume();
#endif
            lastSentNotifications = millis();
        }
    }

    /**
     * Send a notification using the FCM service. This function is not synchronous, it will add the notification to a send queue.
     *
     * @param title Notification title. Max size of EMBER_NOTIFICATION_MAX_TITLE_SIZE.
     * @param text Notification text. Max size of EMBER_NOTIFICATION_MAX_TEXT_SIZE.
     * @param soundId
     * @param soundDurationSeconds
     * @param loopSound
     * @return See the EmberSendNotificationStatus enum for cases.
     */
    EmberSendNotificationStatus send(
        const char *title,
        const char *text,
        const int soundId = -1,
        const uint16_t soundDurationSeconds = 60,
        const bool loopSound = false)
    {
        if (currentNotification >= EMBER_NOTIFICATION_QUEUE_SIZE)
        {
            HTTP_LOGN("Notification queue is full.");
            return EMBER_NOTIF_QUEUE_FULL;
        }

        if (strlen(title)+1 > EMBER_NOTIFICATION_MAX_TITLE_SIZE)
        {
            HTTP_LOGN("Title too big.");
            return EMBER_NOTIF_TITLE_TOO_BIG;
        }

        if (strlen(text)+1 > EMBER_NOTIFICATION_MAX_TEXT_SIZE)
        {
            HTTP_LOGN("Text too big.");
            return EMBER_NOTIF_TEXT_TOO_BIG;
        }

        strcpy(notificationQueue[currentNotification].title, title);
        strcpy(notificationQueue[currentNotification].text, text);

        notificationQueue[currentNotification].soundId = soundId;
        notificationQueue[currentNotification].soundDurationSeconds = soundDurationSeconds;
        notificationQueue[currentNotification].soundLoop = loopSound;

        currentNotification++;
        return EMBER_NOTIF_QUEUED;
    }

private:

    /**
     * Initializes this class. Should be called before loop starts.
     * @param userUid Firebase user uid.
     * @param emberInstance EmberIot instance.
     */
    void doInit(const char *userUid, EmberIot *emberInstance)
    {
        initCalled = true;
        FirePropUtil::initTime();

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

        char expLocation[strlen(littleFsTempTokenLocation)+5];
        sprintf(expLocation, "%s-exp", littleFsTempTokenLocation);
        if (LittleFS.exists(expLocation))
        {
            File expFile = LittleFS.open(expLocation, "r");
            tokenExpiration = expFile.readString().toInt();
            expFile.close();
        }
#endif

        if (userUid != nullptr)
        {
            strcpy(this->userUid, userUid);
            uidInit = true;
        }
        this->emberInstance = emberInstance;

        if (emberInstance != nullptr)
        {
            clientPtr = emberInstance->getWifiClient();
        }

        if (clientPtr == nullptr)
        {
            auto ch = new WithSecureClient();
            clientPtr = &ch->client;
        }
    }

    void sendPending()
    {
        if (!uidInit)
        {
            return;
        }

        if (currentNotification == 0 || forceRenew)
        {
            return;
        }

#ifdef EMBER_STORAGE_USE_LITTLEFS
        if (!LittleFS.exists(this->littleFsTempTokenLocation))
        {
            HTTP_LOGN("Auth file does not exist, trying again later.");
            return;
        }
#else
        if (strlen(currentToken) == 0)
        {
            return;
        }
#endif

        HTTP_LOGN("Send pending notifications.");

        EmberIotNotification notif = notificationQueue[currentNotification-1];

        size_t soundSettingsSize = 0;
        if (notif.soundId >= 0)
        {
            soundSettingsSize = strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_5) +
                strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_DURATION_6) +
                strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_LOOP_7) +
                snprintf(nullptr, 0, "%d", notif.soundId) +
                snprintf(nullptr, 0, "%d", notif.soundDurationSeconds) +
                (notif.soundLoop ? 4 : 5); // true = 4; false = 5
        }

        size_t contentLength = strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_TOPIC_1) +
            strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_TITLE_2) +
            strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_TEXT_3) +
            (deviceId != nullptr ? strlen(deviceId) : 0) +
            (deviceId != nullptr ? strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_DEVID_4) : 0) +
            (deviceName != nullptr ? strlen(deviceName) + 3 : 0) +
            soundSettingsSize +
            strlen_P(EmberIotNotificationValues::SEND_NOTIF_BODY_END) +
            strlen(userUid) +
            strlen(notif.text) +
            strlen(notif.title);

        WiFiClientSecure &client = *clientPtr;

        char host[strlen_P(EmberIotNotificationValues::SEND_NOTIF_HOST)+1];
        strcpy_P(host, EmberIotNotificationValues::SEND_NOTIF_HOST);

        if (!HTTP_UTIL::connectToHost(host, client))
        {
            HTTP_LOGN("Error while connecting, trying again later.");
            return;
        }
        HTTP_UTIL::printHttpProtocol(FPSTR(EmberIotNotificationValues::SEND_NOTIF_PATH), FPSTR(HTTP_UTIL::METHOD_POST), client);
        HTTP_UTIL::printHost(host, client);
        HTTP_UTIL::printContentType(client);

        HTTP_PRINT_BOTH("Authorization: Bearer ", client);

#ifdef EMBER_STORAGE_USE_LITTLEFS
        File tokenFile = LittleFS.open(littleFsTempTokenLocation, "r");
        HTTP_UTIL::printChunked(tokenFile, client);
        client.println();
        tokenFile.close();
#else
        HTTP_PRINT_BOTH(currentToken, client);
        HTTP_PRINT_LN(client);
        HTTP_LOGN();
#endif

        HTTP_UTIL::printContentLengthAndEndHeaders(contentLength, client);

        EMBER_DEBUGN("Body: ");
        HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_TOPIC_1), client);
        HTTP_PRINT_BOTH(userUid, client);

        HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_TITLE_2), client);
        if (deviceName != nullptr)
        {
            HTTP_PRINT_BOTH(deviceName, client);
            HTTP_PRINT_BOTH(" - ", client);
        }
        HTTP_PRINT_BOTH(notif.title, client);

        HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_TEXT_3), client);
        HTTP_PRINT_BOTH(notif.text, client);

        if (deviceId != nullptr)
        {
            HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_DEVID_4), client);
            HTTP_PRINT_BOTH(deviceId, client);
        }

        if (notif.soundId >= 0)
        {
            HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_5), client);
            HTTP_PRINT_BOTH(notif.soundId, client);

            HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_DURATION_6), client);
            HTTP_PRINT_BOTH(notif.soundDurationSeconds, client);

            HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_SOUND_LOOP_7), client);
            HTTP_PRINT_BOTH(notif.soundLoop ? "true" : "false", client);
        }

        HTTP_PRINT_BOTH(FPSTR(EmberIotNotificationValues::SEND_NOTIF_BODY_END), client);
        EMBER_DEBUGN();

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        HTTP_LOGF("Response status: %d\n", responseStatus);
        client.stop();

        if (responseStatus <= 0)
        {
            HTTP_LOGF("Send notification request error: %d\n", responseStatus);
            return;
        }

        if (responseStatus == 401 || responseStatus == 403)
        {
            HTTP_LOGN("Request not authorized, trying again later.");
            lastExpirationCheck = millis() - 2000;
            forceRenew = true;
        }

        if (HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGN("Notification sent successfully.");
            currentNotification--;
        }
        else
        {
            HTTP_LOGF("Send notification request error: %d\n", responseStatus);
        }
    }

    bool renewToken()
    {
        size_t privateKeySize = strlen_P((PGM_P) gcmAccountPrivateKey)+1;

#ifdef ESP32
        char *privateKeyBuf = (char*) malloc(privateKeySize);
        if (privateKeyBuf == nullptr)
        {
            HTTP_LOGN("Out of heap for private key, trying again later.");
            return false;
        }

        strcpy_P(privateKeyBuf, gcmAccountPrivateKey);
#endif

        size_t emailSize = strlen_P(gcmAccountEmail);
        char email[emailSize+1];
        strcpy_P(email, gcmAccountEmail);

        time_t now;
        tm timeinfo;
        getLocalTime(&timeinfo);
        time(&now);

#ifdef ESP32
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        const char bodyPattern[] = R"({"scope":"https://www.googleapis.com/auth/firebase.messaging","aud":"https://oauth2.googleapis.com/token","iss":"%s","exp":"%lld","iat":"%lld"})";
#else
        const char bodyPattern[] = R"({"scope":"https://www.googleapis.com/auth/firebase.messaging","aud":"https://oauth2.googleapis.com/token","iss":"%s","exp":"%ld","iat":"%ld"})";
#endif
#elif ESP8266
        const char bodyPattern[] = R"({"scope":"https://www.googleapis.com/auth/firebase.messaging","aud":"https://oauth2.googleapis.com/token","iss":"%s","exp":"%lld","iat":"%lld"})";
#endif

        size_t finalBodySize = strlen(bodyPattern) + emailSize + (20*2) + 1;
        char *bodyBuf = (char*) malloc(finalBodySize);
        if (bodyBuf == nullptr)
        {
            HTTP_LOGN("Out of heap for JWT payload, trying again later.");
#ifdef ESP32
            free(privateKeyBuf);
#endif
            return false;
        }

        snprintf(bodyBuf,
            finalBodySize,
            bodyPattern,
            email,
            now+3600,
            now);

        size_t bufSize = (int)(privateKeySize*1.2) +
            EmberIotNotificationValues::GRANT_STR_SIZE +
            EmberIotNotificationValues::JWT_HEADER_B64_SIZE +
            (int)(finalBodySize*1.5) + 8;
        char *buf = (char*) malloc(bufSize);
        if (buf == nullptr)
        {
            HTTP_LOGN("Out of heap for JWT buffer, trying again later.");
#ifdef ESP32
            free(privateKeyBuf);
#endif
            free(bodyBuf);
            return false;
        }

        strcpy_P(buf, EmberIotNotificationValues::GRANT_STR);
        strcat_P(buf, EmberIotNotificationValues::JWT_HEADER_B64);

        size_t written = 0;
        size_t bodyIndex = EmberIotNotificationValues::JWT_HEADER_B64_SIZE + EmberIotNotificationValues::GRANT_STR_SIZE;
        base64encode((unsigned char*) buf+bodyIndex,
            bufSize-bodyIndex-1,
            &written,
            (unsigned char*) bodyBuf,
            strlen(bodyBuf));

        free(bodyBuf);

        int removedChars = base64urlencode(buf);
        written = written - removedChars;
        buf[bodyIndex+written] = 0;

#ifdef ESP32
        signRS256(buf+EmberIotNotificationValues::GRANT_STR_SIZE, privateKeyBuf, buf + bodyIndex + written + 1, bufSize - bodyIndex - written);
        free(privateKeyBuf);
#elif ESP8266
        signRS256(buf+EmberIotNotificationValues::GRANT_STR_SIZE, (PGM_P) gcmAccountPrivateKey, buf + bodyIndex + written + 1, bufSize - bodyIndex - written);
#endif

        buf[bodyIndex+written] = '.';
        removedChars = base64urlencode(buf);
        buf[strlen(buf)-removedChars] = 0;
        strcat(buf, "\"}");

        HTTP_LOGF("Fetching token for notification auth with body:\n%s\n", buf);

        WiFiClientSecure &client = *clientPtr;

        char hostBuf[strlen_P(EmberIotNotificationValues::AUTH_HOST)+1];
        strcpy_P(hostBuf, EmberIotNotificationValues::AUTH_HOST);

        if (!HTTP_UTIL::connectToHost(hostBuf, client))
        {
            HTTP_LOGN("Connection failed, retrying later.");
            return false;
        }

        HTTP_UTIL::printHttpProtocol(FPSTR(EmberIotNotificationValues::AUTH_PATH), FPSTR(HTTP_UTIL::METHOD_POST), client);
        HTTP_UTIL::printHost(hostBuf, client);
        HTTP_UTIL::printContentType(client);

        size_t bodySize = strlen(buf);
        HTTP_UTIL::printContentLengthAndEndHeaders(bodySize, client);

        EMBER_DEBUGF("Body (length %zu):\n", bodySize);
        HTTP_PRINT_BOTH(buf, client);
        EMBER_DEBUGN();
        free(buf);

        int responseStatus = HTTP_UTIL::getStatusCode(client);
        if (responseStatus == 0 || !HTTP_UTIL::isSuccess(responseStatus))
        {
            HTTP_LOGF("Error while trying to generate notifications token: %d\n", responseStatus);
            client.stop();
            return false;
        }

        if (!client.find(R"("access_token":")"))
        {
            HTTP_LOGN("Token not found in response for notification auth.");
            client.stop();
            return false;
        }

#ifdef EMBER_STORAGE_USE_LITTLEFS
        File tokenFile = LittleFS.open(this->littleFsTempTokenLocation, "w");
        if (!tokenFile)
        {
            HTTP_LOGN("Failed to open token file, cancelling renew.");
            client.stop();
            return false;
        }
        HTTP_UTIL::printChunkedUntil(client, tokenFile, R"(")");
        tokenFile.close();
        client.stop();

        tokenExpiration = now + 3400;
        char expLocation[strlen(littleFsTempTokenLocation)+5];
        sprintf(expLocation, "%s-exp", littleFsTempTokenLocation);
        File expFile = LittleFS.open(expLocation, "w");
        expFile.print(tokenExpiration);
        expFile.close();

#ifdef EMBER_ENABLE_LOGGING
        tokenFile = LittleFS.open(this->littleFsTempTokenLocation, "r");
        expFile = LittleFS.open(expLocation, "r");
        HTTP_LOGF("Saved token successfully (expiration %lu): %s\n", expFile.readString().toInt(), tokenFile.readString().c_str());
        tokenFile.close();
        expFile.close();
#endif
#else
        size_t read = client.readBytesUntil('"', currentToken, sizeof(currentToken)-1);
        EMBER_DEBUGF("Read %zu bytes from stream\n", read);
        currentToken[read < sizeof(currentToken)-1 ? read : sizeof(currentToken)-1] = 0;
        tokenExpiration = now + 3400;

        HTTP_LOGF("Notif token read into memory: %s\n", currentToken);
        HTTP_LOGF("Notif token expiration: %lu\n", tokenExpiration);
        client.stop();
#endif
        return true;
    }

    const char *gcmAccountEmail;
    const char *gcmAccountPrivateKey;

#ifdef EMBER_STORAGE_USE_LITTLEFS
    char *littleFsTempTokenLocation;
#else
    char currentToken[1024]{};
#endif

    time_t tokenExpiration;
    unsigned long lastExpirationCheck;
    unsigned long lastSentNotifications;
    bool forceRenew;

    bool initCalled;
    bool uidInit;
    char userUid[64];
    EmberIot *emberInstance;
    WiFiClientSecure *clientPtr;

    uint8_t currentNotification;
    EmberIotNotification notificationQueue[EMBER_NOTIFICATION_QUEUE_SIZE];

    char *deviceId;
    char *deviceName;
};

#endif
