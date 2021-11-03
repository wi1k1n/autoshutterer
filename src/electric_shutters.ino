/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
  Arduino IDE example: Examples > Arduino OTA > BasicOTA.ino
  Sourse: https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
*********/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "Servo.h"
#include <EncButton.h>
#include <TimerLED.h>
#include <EEPROM.h>

#include "ota.h"
#include "credentials.h"
#include "calibration.h"
#include "externalsignal.h"

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int PIN_LED = D4;
const int PIN_SERVO = D6;
const int PIN_BTN = D5;
const int EEPROM_SIZE = 1024;

enum {
  TASK_OFF,
  TASK_EXTERNAL,
  TASK_INTERNAL,
  TASK_CALIBRATION
};

Servo servo;
int task;  // 0: off, 1: external signals, 2: internal schedule, 3: calibration
bool boundariesInitialized;

TimerLED timer_led(PIN_LED, true);
bool led_state;
EncButton <EB_TICK, PIN_BTN> btn;
ExternalServer srvExternal(servo, PIN_SERVO);
CalibrationServer srvCalib(servo, PIN_SERVO, [](uint8_t _l, uint8_t _c, uint8_t _r) {
  EEPROM.begin(EEPROM_SIZE);
  if (!_l && !_c && !_r) {
    EEPROM.put(0, 0);
    boundariesInitialized = false;
    Serial.print(F("[EEPROM] Boundaries cleared from EEPROM"));
  } else {
    EEPROM.put(0, 42);
    EEPROM.put(1, _l);
    EEPROM.put(2, _c);
    EEPROM.put(3, _r);
    boundariesInitialized = true;
    Serial.print(F("[EEPROM] Servo calibration saved: "));
    Serial.print((int)_l);
    Serial.print(F("; "));
    Serial.print((int)_c);
    Serial.print(F("; "));
    Serial.println((int)_r);
  }
  EEPROM.commit();
  EEPROM.end();
});
void InitializeServoBoundariesFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  uint8_t _startByte, _l, _c, _r;
  EEPROM.get(0, _startByte);
  if (_startByte == 42) {
    EEPROM.get(1, _l);
    EEPROM.get(2, _c);
    EEPROM.get(3, _r);

    boundariesInitialized = true;
    srvCalib.setBoundaries(_l, _c, _r);
    srvExternal.setBoundaries(_l, _c, _r);

    Serial.print(F("[EEPROM] Servo calibration restored: "));
    Serial.print((int)_l);
    Serial.print(F("; "));
    Serial.print((int)_c);
    Serial.print(F("; "));
    Serial.println((int)_r);
  } else {
    boundariesInitialized = false;
    Serial.println(F("[EEPROM] StartByte is not 42. No calibration restored"));
  }
  EEPROM.end();
}

void StartExternal() {
  srvExternal.start();
}
void StopExternal() {
  srvExternal.stop();
}

void StartCalibration() {
  srvCalib.start();
}
void StopCalibration() {
  srvCalib.stop();
}

void changeTask(const int& _task) {
  task = _task;

  if (task != TASK_CALIBRATION) {
    StopCalibration();
  }
  if (task != TASK_EXTERNAL) {
    StopExternal();
  }
  
  switch (task) {
    case TASK_OFF:
      // timer_led.setIntervals(2925, 25);
      timer_led.stop();
      break;
    case TASK_EXTERNAL:
      timer_led.setIntervals(5, (const uint16_t[]){725, 25, 725, 25, 5000-725});
      timer_led.restart();
      StartExternal();
      break;
    case TASK_INTERNAL:
      timer_led.setIntervals(100, 250, 1600, 50);
      timer_led.restart();
      break;
    case TASK_CALIBRATION:
      timer_led.setIntervals(100, 100);
      timer_led.restart();
      StartCalibration();
      break;
    default:
      break;
  }
  Serial.print("Task changed to #");
  Serial.println(task);
}




void configureWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}


/*
#################
##### SETUP #####
#################
*/
void setup() {
  Serial.begin(115200);
  Serial.println("[ESP] Booting");
  
  InitializeServoBoundariesFromEEPROM();
  changeTask(boundariesInitialized ? TASK_EXTERNAL : TASK_CALIBRATION);

  configureWiFi();
  OTA::setup();
  
  Serial.println("[ESP] Ready");
  Serial.print("[ESP] IP address: ");
  Serial.println(WiFi.localIP());
}


/*
#################
###### LOOP #####
#################
*/
void loop() {
  OTA::handle();
  btn.tick();
  timer_led.tick();

  if (btn.press()) {  
    task = (task + 1) % 4;
    changeTask(task);
  }

  switch (task) {
    case TASK_OFF:
      break;
    case TASK_EXTERNAL:
      srvExternal.listen();
      break;
    case TASK_INTERNAL:
      break;
    case TASK_CALIBRATION:
      srvCalib.listen();
      break;
    default:
      break;
  }
}