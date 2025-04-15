//
// Created by xav on 3/28/25.
//

#ifndef CERTIFICATES_H
#define CERTIFICATES_H

#include <WiFiClientSecure.h>
#include <cert/google_root_ca.h>

namespace EmberIotCertificates
{
    bool init = false;

#ifdef ESP32
    extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
    extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#elif ESP8266
    inline X509List cert(google_root_ca);
#endif

    inline void addCertificatesToClient(WiFiClientSecure &client)
    {
#ifdef ESP8266
        if (!init)
        {
            configTime(0, 0, "pool.ntp.org");
            init = true;
        }
#endif

#ifdef ESP32
        client.setCACertBundle(x509_crt_imported_bundle_bin_start);
#elif ESP8266
        client.setTrustAnchors(&cert);
#endif
    }
}

#endif //CERTIFICATES_H
