//
// Created by xav on 3/28/25.
//

#ifndef WITHCLIENT_H
#define WITHCLIENT_H

#include <EmberIotCertificates.h>
#include <WiFiClientSecure.h>

class WithSecureClient {
public:
    WithSecureClient()
    {
        EmberIotCertificates::addCertificatesToClient(client);
    }
protected:
    WiFiClientSecure client{};
};

#endif //WITHCLIENT_H
