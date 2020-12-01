#ifndef DEVICE_DB_H
#define DEVICE_DB_H

#include <ArduinoJson.h>
#ifdef ESP8266
#include <LittleFS.h>
#endif
#ifdef ESP32
#include <LITTLEFS.h>
#endif

#include "global_defines.h"

#define RECORD_SIZE 256

class DeviceDB {
public:
	DeviceDB(FS* fileSystem);
	~DeviceDB();

    bool init(String database);
    bool check();

    String database;

    byte device();
    void device(byte device);
    byte drive();
    void drive(byte drive);
    byte partition();
    void partition(byte partition);
    String url();
    void url(String url);
    String path();
    void path(String path);
    String image();
    void image(String disk);

    bool select(byte device);

private:
    bool m_dirty;
    FS* m_fileSystem;
    StaticJsonDocument<256> m_device;
};

#endif