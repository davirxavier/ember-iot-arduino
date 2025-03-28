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
    const char *AUTH_URL PROGMEM = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
    const char *AUTH_HOST PROGMEM = "identitytoolkit.googleapis.com";
    const uint16_t AUTH_URL_SIZE PROGMEM = strlen(AUTH_URL);

    const char *REFRESH_URL PROGMEM = "https://securetoken.googleapis.com/v1/token?key=";
    const char *REFRESH_HOST PROGMEM = "securetoken.googleapis.com";
    const uint16_t REFRESH_URL_SIZE PROGMEM = strlen(REFRESH_URL);
}

class EmberIotAuth : public WithSecureClient
{
public:
    EmberIotAuth(const char* username, const char* password, const char* apiKey): username(username), password(password), apiKey(apiKey)
    {
        this->apiKeySize = strlen(apiKey);
        this->refreshTokenSet = false;
        this->tokenExpiry = 0;
        this->tokenSize = 0;
        this->lastTry = 0;
    }

    bool isExpired() const
    {
        return millis() > tokenExpiry;
    }

    char *getUserUid()
    {
        if (!refreshTokenSet)
        {
            getToken();
        }

        return userUid;
    }

    char *getToken()
    {
        if (!refreshTokenSet || strlen(refreshToken) == 0)
        {
            HTTP_LOGN("Token not set, authenticating.");
            if (authenticateFirebase())
            {
                return currentToken;
            }
        }

        if (isExpired())
        {
            HTTP_LOGN("Token expired.");
            if (refreshFirebaseToken() || authenticateFirebase())
            {
                return currentToken;
            }
        }

        return this->currentToken;
    }

    uint16_t getTokenSize() const
    {
        return tokenSize;
    }

    void loop()
    {
        if ((isExpired() || !this->refreshTokenSet) && millis() - lastTry > 5000)
        {
            if (!refreshFirebaseToken())
            {
                authenticateFirebase();
            }

            lastTry = millis();
        }
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
        strcpy(url, EmberIotAuthValues::AUTH_URL);
        strcat(url, this->apiKey);

        JsonDocument responseDoc;
        JsonDocument filter;
        filter["idToken"] = true;
        filter["refreshToken"] = true;
        filter["expiresIn"] = true;
        filter["localId"] = true;

        bool result = HTTP_UTIL::doJsonHttpRequest(this->client,
            url,
            EmberIotAuthValues::AUTH_HOST,
            HTTP_UTIL::METHOD_POST,
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

        strcpy(refreshToken, responseDoc["refreshToken"]);
        tokenExpiry = millis() + ((responseDoc["expiresIn"].as<unsigned long>() - 300) * 1000);
        refreshTokenSet = true;

        strcpy(userUid, responseDoc["localId"]);
        HTTP_LOGF("User uid: %s\n", userUid);
        HTTP_LOGF("Token expires in %u millis.\n", tokenExpiry);
        return true;
    }

    bool refreshFirebaseToken()
    {
        if (this->refreshToken == nullptr || strlen(this->refreshToken) == 0)
        {
            return false;
        }

        JsonDocument doc;
        doc["grant_type"] = "refresh_token";
        doc["refresh_token"] = this->refreshToken;

        unsigned int requestBodySize = measureJson(doc);
        char requestBody[requestBodySize + 1];
        serializeJson(doc, requestBody, requestBodySize+1);

        char url[EmberIotAuthValues::REFRESH_URL_SIZE + this->apiKeySize + 1];
        strcpy(url, EmberIotAuthValues::REFRESH_URL);
        strcat(url, this->apiKey);

        JsonDocument responseDoc;
        JsonDocument filter;
        filter["id_token"] = true;
        filter["expires_in"] = true;

        bool result = HTTP_UTIL::doJsonHttpRequest(this->client,
            url,
            EmberIotAuthValues::REFRESH_HOST,
            HTTP_UTIL::METHOD_POST,
            requestBody,
            &filter,
            &responseDoc);

        if (!result)
        {
            HTTP_LOGN("Firebase refresh token request has invalid response.");
            return false;
        }

        strcpy(this->currentToken, responseDoc["id_token"]);
        this->tokenSize = strlen(currentToken);

        tokenExpiry = millis() + (responseDoc["expires_in"].as<unsigned long>() * 1000);
        return true;
    }

    const char* username;
    const char* password;
    const char* apiKey;
    uint16_t apiKeySize;

    char userUid[36]{};
    bool refreshTokenSet;
    char currentToken[1024]{};
    uint16_t tokenSize;
    char refreshToken[256]{};
    unsigned long tokenExpiry;
    unsigned long lastTry;
};

#endif //FIREBASEAUTH_H
