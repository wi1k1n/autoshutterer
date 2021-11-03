#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTA {
public:
  static void setup(const char* password = "", const char* hostname = "", uint32_t port = 8266) {
    // Port defaults to 8266
    Serial.print(F("\n[OTA] Set port to: "));
    Serial.println(port);
    ArduinoOTA.setPort(port);
  
    // Hostname defaults to esp8266-[ChipID]
    if (hostname) {
      Serial.print(F("[OTA] Set hostname to: "));
      Serial.println(hostname);
      ArduinoOTA.setHostname((const char *)hostname);
    }
  
    // No authentication by default
    if (password) {
      Serial.print(F("[OTA] Set password to: "));
      Serial.println(password);
      ArduinoOTA.setPassword((const char *)"123");
    }

    _setup();
  }
  static void handle() { ArduinoOTA.handle(); }
private:
  static void _setup() {
    ArduinoOTA.onStart([]() {
      Serial.println("[OTA] Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\n[OTA] End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("[OTA] Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
  }
};

#endif /* OTA_H */
