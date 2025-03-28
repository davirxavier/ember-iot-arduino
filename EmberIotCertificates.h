//
// Created by xav on 3/28/25.
//

#ifndef CERTIFICATES_H
#define CERTIFICATES_H

#include <WiFiClientSecure.h>

namespace EmberIotCertificates
{
    extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
    extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");

    inline void addCertificatesToClient(WiFiClientSecure &client)
    {
        client.setCACertBundle(x509_crt_imported_bundle_bin_start);
    }
}

#endif //CERTIFICATES_H
