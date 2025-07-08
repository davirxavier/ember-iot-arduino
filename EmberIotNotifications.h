#ifndef FIREPROP_NOTIF_H
#define FIREPROP_NOTIF_H

#include <time.h>
#include <EmberIot.h>
#include <EmberIotCryptUtil.h>
#include <LittleFS.h>

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
    const char AUTH_URL[] PROGMEM = "https://oauth2.googleapis.com/token";
    const char AUTH_HOST[] PROGMEM = "oauth2.googleapis.com";
    const uint16_t AUTH_URL_SIZE = strlen_P(AUTH_URL);

    const char SEND_NOTIF_URL[] PROGMEM = "https://fcm.googleapis.com/v1/projects/ember-iot/messages:send";
    const char SEND_NOTIF_HOST[] PROGMEM = "fcm.googleapis.com";
    const uint16_t SEND_NOTIF_URL_SIZE = strlen_P(SEND_NOTIF_URL);

    const char GRANT_STR[] PROGMEM = R"({"grant_type": "urn:ietf:params:oauth:grant-type:jwt-bearer","assertion":")";
    const uint16_t GRANT_STR_SIZE = strlen_P(GRANT_STR);

    const char JWT_HEADER_B64[] PROGMEM = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.";
    const uint16_t JWT_HEADER_B64_SIZE = strlen_P(JWT_HEADER_B64);

    const char ACCESS_TOKEN_DELIMITER[] PROGMEM = R"("access_token":")";
}

struct EmberIotNotification
{
    char title[EMBER_NOTIFICATION_MAX_TITLE_SIZE];
    char text[EMBER_NOTIFICATION_MAX_TEXT_SIZE];
};

class FCMEmberNotifications : WithSecureClient
{
public:
    /**
     * New notifications instance.
     * @param gcmAccountEmail Google service account e-mail. Should be a flash string.
     *      DON'T USE THE FIREBASE MAIN SERVICE ACCOUNT, IT'S A VERY BIG SECURITY RISK, AS IT HAS EVERY POSSIBLE PERMISSION.
     * @param gcmAccountPrivateKey Google service account primary key. Should be a flash string.
     * @param littleFsTempTokenLocation Where to persist the generated oauth token in storage. Default is "/ember-iot-temp/notif-token".
     */
    FCMEmberNotifications(
        const __FlashStringHelper *gcmAccountEmail,
        const __FlashStringHelper *gcmAccountPrivateKey,
        const char *littleFsTempTokenLocation = "/ember-iot-temp/notif-token"
    ):
        gcmAccountEmail(gcmAccountEmail),
        gcmAccountPrivateKey(gcmAccountPrivateKey),
        tokenExpiration(0),
        lastExpirationCheck(0),
        lastSentNotifications(0),
        currentNotification(0),
        notificationQueue{},
        forceRenew(false),
        userUid{},
        emberInstance(nullptr),
        uidInit(false)
    {
        size_t fileSize = strlen(littleFsTempTokenLocation);
        this->littleFsTempTokenLocation = (char*)malloc(fileSize + 1);
        strcpy(this->littleFsTempTokenLocation, littleFsTempTokenLocation);

        this->littleFsTempTokenExpiration = (char*)malloc(fileSize + 5);
        strcpy(this->littleFsTempTokenExpiration, littleFsTempTokenLocation);
        strcat(this->littleFsTempTokenExpiration, "-exp");
    }

    ~FCMEmberNotifications()
    {
        free(this->littleFsTempTokenLocation);
    }

    /**
     * Initializes this class using an userUid directly. Should be called before loop starts.
     * @param userUid Firebase user uid.
     */
    void init(const char *userUid)
    {
        init(userUid, nullptr);
    }

    /**
     * Initialized this class using an EmberIot instance. Should be called before loop starts.
     * @param emberInstance
     */
    void init(EmberIot *emberInstance)
    {
        init(nullptr, emberInstance);
    }

    /**
     * Should be called every loop.
     */
    void loop()
    {
        if (!FirePropUtil::isTimeInitialized())
        {
            return;
        }

        if (emberInstance != nullptr && !uidInit && emberInstance->getUserUid() != nullptr)
        {
            uidInit = true;
            strcpy(userUid, emberInstance->getUserUid());
        }

        if (millis() - lastExpirationCheck > 2000)
        {
            time_t now;
            time(&now);

            if (now > tokenExpiration || forceRenew)
            {
                HTTP_LOGN("Notification token expired, renewing.");
                renewToken();
                forceRenew = false;
            }

            lastExpirationCheck = millis();
        }

        if (millis() - lastSentNotifications > 100)
        {
            sendPending();
            lastSentNotifications = millis();
        }
    }

    /**
     * Send a notification using the FCM service. This function is not synchronous, it will add the notification to a queue.
     *
     * @param title Notification title. Max size of EMBER_NOTIFICATION_MAX_TITLE_SIZE.
     * @param text Notification text. Max size of EMBER_NOTIFICATION_MAX_TEXT_SIZE.
     * @return false if queue is full or title and text are too big, true if success.
     */
    bool send(const char *title, const char *text)
    {
        if (currentNotification >= EMBER_NOTIFICATION_QUEUE_SIZE)
        {
            HTTP_LOGN("Notification queue is full.");
            return false;
        }

        if (strlen(title)+1 > EMBER_NOTIFICATION_MAX_TITLE_SIZE)
        {
            HTTP_LOGN("Title too big.");
            return false;
        }

        if (strlen(text)+1 > EMBER_NOTIFICATION_MAX_TEXT_SIZE)
        {
            HTTP_LOGN("Text too big.");
            return false;
        }

        strcpy(notificationQueue[currentNotification].title, title);
        strcpy(notificationQueue[currentNotification].text, text);
        currentNotification++;
        return true;
    }

private:
    void init(const char *userUid, EmberIot *emberInstance)
    {
        FirePropUtil::initTime();

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

        if (LittleFS.exists(littleFsTempTokenExpiration))
        {
            File expFile = LittleFS.open(littleFsTempTokenExpiration, "r");
            tokenExpiration = expFile.readString().toInt();
            expFile.close();
        }

        if (userUid != nullptr)
        {
            strcpy(this->userUid, userUid);
            uidInit = true;
        }
        this->emberInstance = emberInstance;
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

        if (!LittleFS.exists(this->littleFsTempTokenLocation))
        {
            HTTP_LOGN("Auth file does not exist, trying again later.");
            return;
        }

        EmberIotNotification notif = notificationQueue[currentNotification-1];

        JsonDocument doc;

        JsonObject message = doc[F("message")].to<JsonObject>();
        message[F("topic")] = userUid;

        JsonObject message_notification = message[F("notification")].to<JsonObject>();
        message_notification[F("title")] = notif.title;
        message_notification[F("body")] = notif.text;
        message[F("android")][F("priority")] = F("high");

        doc.shrinkToFit();

        File authFile = LittleFS.open(this->littleFsTempTokenLocation, "r");

        int responseStatus = 0;

        HTTP_LOGN("Sending notification.");
        bool completed = HTTP_UTIL::doChunkedHttpRequest(client,
            EmberIotNotificationValues::SEND_NOTIF_URL,
            EmberIotNotificationValues::SEND_NOTIF_HOST,
            FPSTR(HTTP_UTIL::METHOD_POST),
            [&doc, &authFile](WiFiClientSecure &client)
            {
                client.print(F("Authorization: Bearer "));
                char buffer[64];
                while (authFile.available())
                {
                    size_t read = authFile.readBytes(buffer, 64);
                    for (size_t i = 0; i < read; i++)
                    {
                        client.print(buffer[i]);
                    }
                }
                client.println();
                HTTP_UTIL::printContentLength(client, measureJson(doc), buffer, 64);
                serializeJson(doc, client);

                HTTP_LOGN("Notification request body:");
#ifdef EMBER_ENABLE_LOGGING
                serializeJson(doc, Serial);
                Serial.println();
#endif
            },
            [&responseStatus](const uint8_t buf[64], uint8_t length)
            {
                responseStatus = HTTP_UTIL::getStatusCode(buf, length);
                HTTP_LOGF("Status: %d\n", responseStatus);
                return false;
            });

        if (responseStatus <= 0 || !completed)
        {
            HTTP_LOGN("Request error.");
            return;
        }

        if (responseStatus == 401 || responseStatus == 403)
        {
            HTTP_LOGN("Request not authorized, trying again later.");
            lastExpirationCheck = millis() - 2000;
            forceRenew = true;
        }

        if (responseStatus == 200)
        {
            HTTP_LOGN("Notification sent successfully.");
            currentNotification--;
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

        strcpy_P(privateKeyBuf, (PGM_P) gcmAccountPrivateKey);
#endif


        size_t emailSize = strlen_P((PGM_P) gcmAccountEmail);
        char email[emailSize+1];
        strcpy_P(email, (PGM_P) gcmAccountEmail);

        time_t now;
        tm timeinfo;
        getLocalTime(&timeinfo);
        time(&now);

#ifdef ESP32
        const char bodyPattern[] = R"({"scope":"https://www.googleapis.com/auth/firebase.messaging","aud":"https://oauth2.googleapis.com/token","iss":"%s","exp":"%ld","iat":"%ld"})";
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

        size_t delimiterSize = strlen_P(EmberIotNotificationValues::ACCESS_TOKEN_DELIMITER);
        char delimiter[delimiterSize+1];
        strcpy_P(delimiter, EmberIotNotificationValues::ACCESS_TOKEN_DELIMITER);
        size_t currentDelimiterChar = 0;

        File tokenFile = LittleFS.open(this->littleFsTempTokenLocation, "w");
        if (!tokenFile)
        {
            HTTP_LOGN("Failed to open token file, cancelling renew.");
            return false;
        }

        HTTP_UTIL::doChunkedHttpRequest(client,
            EmberIotNotificationValues::AUTH_URL,
            EmberIotNotificationValues::AUTH_HOST,
            FPSTR(HTTP_UTIL::METHOD_POST),
            [bodyBuf, finalBodySize, buf](WiFiClientSecure &client)
            {
                HTTP_UTIL::printContentLength(client, strlen(buf), bodyBuf, finalBodySize);
                client.print(buf);
            },
            [&delimiter, &currentDelimiterChar, &delimiterSize, &tokenFile](const uint8_t buf[64], uint8_t length)
            {
                for (int i = 0; i < length; i++)
                {
                    if (currentDelimiterChar >= delimiterSize)
                    {
                        if (buf[i] == '"')
                        {
                            return false;
                        }

                        tokenFile.print((char)buf[i]);
                        continue;
                    }

                    if (isspace(buf[i]))
                    {
                        continue;
                    }

                    if (buf[i] == delimiter[currentDelimiterChar])
                    {
                        currentDelimiterChar++;
                    }
                    else
                    {
                        currentDelimiterChar = 0;
                    }
                }

                return true;
            });

        tokenFile.close();
        free(buf);
        free(bodyBuf);

        if (currentDelimiterChar < delimiterSize)
        {
            HTTP_LOGN("Access token not found in response object.");
            return false;
        }

        tokenExpiration = now + 3400;
        File expFile = LittleFS.open(littleFsTempTokenExpiration, "w");
        expFile.print(tokenExpiration);
        expFile.close();
        HTTP_LOGN("Saved token successfully.");
        return true;
    }

    const __FlashStringHelper *gcmAccountEmail;
    const __FlashStringHelper *gcmAccountPrivateKey;
    char *littleFsTempTokenLocation;
    char *littleFsTempTokenExpiration;
    time_t tokenExpiration;
    unsigned long lastExpirationCheck;
    unsigned long lastSentNotifications;
    bool forceRenew;

    bool uidInit;
    char userUid[64];
    EmberIot *emberInstance;

    uint8_t currentNotification;
    EmberIotNotification notificationQueue[EMBER_NOTIFICATION_QUEUE_SIZE];
};

#endif
