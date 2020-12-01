#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#endif


#include "global_defines.h"

class HTTPUpdate {
public:
    const char* updateURL;

    void checkForUpdates();
    String getMAC();
};