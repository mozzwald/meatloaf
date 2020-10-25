
#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

String urlencode(String str);
String urldecode(String str);
unsigned char h2int(char c);
String ipToString ( IPAddress ip );
String formatBytes ( size_t bytes );
void printProgress(uint16_t total, uint16_t current);

#endif