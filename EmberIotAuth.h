//
// Created by xav on 3/26/25.
//

#ifndef FIREBASEAUTH_H
#define FIREBASEAUTH_H

#include <ArduinoJson.h>
#include <EmberIotHttp.h>
#include <WithSecureClient.h>

namespace EmberIotAuthValues
{
    const char AUTH_URL[] PROGMEM = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
    const char AUTH_HOST[] PROGMEM = "identitytoolkit.googleapis.com";
    const uint16_t AUTH_URL_SIZE = strlen_P(AUTH_URL);
}

class EmberIotAuth : public WithSecureClient
{
public:
    EmberIotAuth(const char* username, const char* password, const char* apiKey): username(username), password(password), apiKey(apiKey)
    {
        this->apiKeySize = strlen(apiKey);
        this->userUidSet = false;
        this->tokenExpiry = 0;
        this->tokenSize = 0;
        this->lastTry = 0;
        currentToken = new char[1024]{};
    }

    bool isExpired() const
    {
        return millis() > tokenExpiry;
    }

    char *getUserUid()
    {
        return userUid;
    }

    char *getToken() const
    {
        return this->currentToken;
    }

    uint16_t getTokenSize() const
    {
        return tokenSize;
    }

    void loop()
    {
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

private:
    bool authenticateFirebase()
    {
        JsonDocument doc;
        doc["email"] = username;
        doc["password"] = password;
        doc["returnSecureToken"] = true;

        unsigned int requestBodySize = measureJson(doc);
        char requestBody[requestBodySize + 1];
        serializeJson(doc, requestBody, requestBodySize+1);

        char url[EmberIotAuthValues::AUTH_URL_SIZE + this->apiKeySize + 1];
        strcpy_P(url, EmberIotAuthValues::AUTH_URL);
        strcat(url, this->apiKey);

        JsonDocument responseDoc;
        JsonDocument filter;
        filter["idToken"] = true;
        filter["refreshToken"] = true;
        filter["expiresIn"] = true;
        filter["localId"] = true;

        char host[strlen_P(EmberIotAuthValues::AUTH_HOST)+1];
        strcpy_P(host, EmberIotAuthValues::AUTH_HOST);

        bool result = HTTP_UTIL::doJsonHttpRequest(this->client,
            url,
            host,
            FPSTR(HTTP_UTIL::METHOD_POST),
            requestBody,
            &filter,
            &responseDoc);

        if (!result)
        {
            HTTP_LOGN("Firebase auth request has invalid response.");
            return false;
        }

        strcpy(currentToken, responseDoc["idToken"]);
        this->tokenSize = strlen(currentToken);

        strcpy(userUid, responseDoc["localId"]);
        userUidSet = true;

        tokenExpiry = millis() + (responseDoc["expiresIn"].as<unsigned long long>() * 1000UL);
        HTTP_LOGF("User uid: %s\n", userUid);
        HTTP_LOGF("Token expires in %lu millis.\n", tokenExpiry);
        return true;
    }

    const char* username;
    const char* password;
    const char* apiKey;
    uint16_t apiKeySize;

    char userUid[36]{};
    bool userUidSet;
    char *currentToken;
    uint16_t tokenSize;
    unsigned long tokenExpiry;
    unsigned long lastTry;
};

#endif //FIREBASEAUTH_H
