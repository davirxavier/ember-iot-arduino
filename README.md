# 🔥 Ember IoT – Host Your Own IoT Cloud

Ember IoT is a simple proof of concept for a Firebase-hosted IoT cloud designed to work with Arduino-based devices and an Android mobile app. It enables microcontrollers to connect to the cloud, securely sync data, and interact with a mobile interface all while using the Firebase Authentication and Firebase Realtime Database services.

**This project's aim is to enable you to create a simple IoT infrastructure without all the hassle of setting up a server in your home network or in a cloud provider.**

<!-- TOC -->
* [🔥 Ember IoT – Host Your Own IoT Cloud](#-ember-iot--host-your-own-iot-cloud)
  * [🔍 What Is Ember IoT?](#-what-is-ember-iot)
  * [❓ Why Would I Use Firebase for IoT?](#-why-would-i-use-firebase-for-iot)
    * [Short Answer:](#short-answer)
    * [Detailed Answer:](#detailed-answer)
* [🔥 Firebase Setup](#-firebase-setup)
* [📱 App Setup](#-app-setup)
* [Arduino Setup](#arduino-setup)
    * [Simple Data Channel Listen and Write Example:](#simple-data-channel-listen-and-write-example)
      * [`EmberIot ember(...)`](#emberiot-ember)
      * [`EMBER_CHANNEL_CB(channel)`](#ember_channel_cbchannel)
      * [`ember.init()`](#emberinit)
      * [`ember.loop()`](#emberloop)
      * [`ember.channelWrite(channel, value)`](#emberchannelwritechannel-value)
  * [How it Works - What even is a Data Channel?](#how-it-works---what-even-is-a-data-channel)
* [📝 TODO](#-todo)
<!-- TOC -->

---

## 🔍 What Is Ember IoT?

Ember IoT allows you to:

- Connect Arduino and similar microcontroller devices to the cloud
- Interact with your devices in real-time using an Android app
- Securely authenticate users and devices with Firebase Authentication
- Store and sync your microcontroller state between devices

This project includes an Android application that features device management and a customizable UI for interacting with connected devices. Alternatively, the Ember IoT library can also be used for microcontroller-to-microcontroller communication, allowing devices to communicate directly without the need for the Android app.

---

## ❓ Why Would I Use Firebase for IoT?

### Short Answer:
Firebase’s free Spark plan from Google seems to be suitable for handling small-scale IoT projects — potentially supporting dozens of devices with real-time data sync and built-in user authentication. This enables you to create a simple IoT infrastructure without all the hassle of setting up a server in your home network or in a cloud provider.

### Detailed Answer:

Most free-tier IoT platforms come with major limitations:

- Device limits (e.g., only a few devices without paying)
- Strict quotas on messages, data storage, or connection time
- High costs to unlock even basic scalability

Firebase, on the other hand, offers:

- Real-time sync with low bandwidth
- Simple and secure authentication with support for email and password
- A relatively generous free tier under the Spark plan:
    - 100 simultaneous database connections (each microcontroller and Android device will use 1 connection slot)
    - 1 GB storage (each device will use less than 1kb of storage)
    - 50,000 reads/day, 20,000 writes/day

This could make Firebase a compelling backend for IoT hobbyists, educators, and developers looking for an easy-to-use platform with minimal upfront cost. You have the flexibility to decide exactly how much you need and tailor your setup accordingly, optimizing it as your project grows. This ensures you’re not bound by the limitations of third-party platforms, giving you full control over your project’s scalability and requirements.

Plus, you don't have to enter any credit card info to create a project :)

---

# 🔥 Firebase Setup

See the [project setup tutorial](FIREBASE_SETUP.md).

---

# 📱 App Setup

Before continuing to the next step, you will need to install the Android app and create/configure a device. [Android app repository](https://github.com/davirxavier/ember-iot-android-app).

Alternatively, if you prefer, you can use this library to facilitate communication between microcontrollers only, without needing the app.

# Arduino Setup

Include the library in your Arduino project. You can download this repository's zip file or include the repository URL in PlatformIO.

### Simple Data Channel Listen and Write Example:

```c++
#include <Arduino.h>
#include <WiFi.h>

// Define the maximum channel count used in the project before importing the library. This number doesn't need to match exactly the used channel count, you can define more channels than you are using.
#define EMBER_CHANNEL_COUNT 5
#include <EmberIot.h>

// LED pin for switching an LED.
#define LED_PIN 1

#define RTDB_URL "https://tutorial-a23d1-default-rtdb.firebaseio.com/"
#define USER "tutorial@gmail.com"
#define PASSWORD "password"
#define WEB_API_KEY "yourwebapikey"

// Device ID that was created in the Android app. 
// Can be any string if using the library for microcontroller-to-microcontroller communication.
// If not using the Android app, this ID needs to be the same between two boards.
#define DEVICE_ID "-OMms-5sj7f6DGsZXvY0"

// Initialize the EmberIot instance
EmberIot ember(RTDB_URL,
                  DEVICE_ID,
                  USER,
                  PASSWORD,
                  WEB_API_KEY);

// Add a callback for when the data channel number 0 is updated
EMBER_CHANNEL_CB(0)
{
    Serial.printf("Received CH0 end event: %s\n", prop.toString() == nullptr ? "null" : prop.toString());

    // 'prop' is the variable containing the field that has changed, you can use the toX() function to convert it to various data types, like below
    int val = prop.toInt(); // Convert to int
    if (val == EMBER_BUTTON_ON) // If the int value corresponds to the button ON, turn on the LED
    {
        digitalWrite(LED_PIN, HIGH);
    }
    else if (val == EMBER_BUTTON_PUSH) // If the int value corresponds to the button PUSH, turn on the LED, wait a few millis, and turn it off again
    {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        ember.channelWrite(EMBER_BUTTON_OFF, EMBER_BUTTON_OFF); // Update the channel with the OFF value, signaling that the button is no longer pressed
    }
    else // If the value corresponds to the button OFF, turn off the LED
    {
        digitalWrite(LED_PIN, LOW);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Init LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Init the Wi-Fi connection
    WiFi.begin("network", "password");
    while (WiFi.status() != WL_CONNECTED)
    {
    }

    Serial.println("Wi-Fi started.");

    // Init the EmberIotClass
    ember.init();
}

void loop()
{
    // Call the loop function in each cycle to maintain the connection to the database
    ember.loop();
}
```

Below is a brief overview of the key functions used in the example code:

#### `EmberIot ember(...)`
This is the constructor that initializes the `EmberIot` instance with the provided parameters:
- `RTDB_URL`: The URL of the Firebase Realtime Database.
- `DEVICE_ID`: A unique identifier for the device (it can be any string, or the ID created in the Android app).
- `USER`: The email address used for Firebase Authentication.
- `PASSWORD`: The password for the Firebase Authentication.
- `WEB_API_KEY`: The Firebase Web API key required to authenticate the connection.

#### `EMBER_CHANNEL_CB(channel)`
This macro is used to define a callback function for a specific data channel. In the example, it’s for channel `0`. The callback function will be called whenever data is received or updated for the specified channel. The parameter `prop` inside the callback represents the data received for that channel.

- `prop.toString()` converts the data to a string format.
- `prop.toInt()` converts the data to an integer.
- `prop.toLong()` converts the data to a long.
- `prop.toLongLong()` converts the data to a long long.
- `prop.toDouble()` converts the data to a double.

In the callback, you can add custom logic to handle the received data, such as controlling hardware like LEDs based on the values received from the cloud.

#### `ember.init()`
This function initializes the EmberIot library and establishes the necessary connections to Firebase Realtime Database. It must be called after setting up the Wi-Fi connection and before starting the main loop to maintain the connection.

#### `ember.loop()`
This function must be called in the `loop()` of the Arduino program. It continuously checks the state of the Firebase Realtime Database to maintain the connection and listen for any updates. This is necessary for the device to interact with the cloud in real-time.

#### `ember.channelWrite(channel, value)`
This function sends data to a specific channel in the Firebase Realtime Database. The first argument, `channel`, is the channel number (e.g., `EMBER_BUTTON_OFF`), and the second argument, `value`, is the data being sent. In the example, it's used to update the channel with the button status (e.g., turning the button OFF).
    
## How it Works - What even is a Data Channel?
    
In this library, a data channel represents a stream of data received from a specific location in the Firebase Realtime Database.

The Firebase Realtime Database REST API supports real-time updates by using the text/event-stream content type over HTTP. When a connection is established to a node (along with authentication), a persistent TCP socket is opened. Any changes to the data at that node are pushed through this connection as they happen.

The library processes this incoming data through data channels, effectively routing each stream into its own variable for clean and organized access in your code.

To send updates, the library uses the same REST API to perform HTTP requests—making it easy for external systems, such as microcontrollers, to write new values to these data channels.

# 📝 TODO

- [ ] Compatibility with ESP8266
- [ ] Notification service