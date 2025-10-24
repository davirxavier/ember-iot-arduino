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

#ifndef CERTIFICATES_H
#define CERTIFICATES_H

#include <WiFiClientSecure.h>
#include <cert/google_root_ca.h>

namespace EmberIotCertificates
{
    bool init = false;

#if ESP8266
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
        client.setCACert(google_root_ca);
#elif ESP8266
        client.setTrustAnchors(&cert);
#endif
    }
}

#endif //CERTIFICATES_H
