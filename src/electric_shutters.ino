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

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int PIN_LED = D4;
const int PIN_SERVO = D6;
const int PIN_BTN = D5;
const int EEPROM_SIZE = 1024;

Servo servo;
int task;  // 0: off, 1: external signals, 2: internal schedule, 3: calibration
int offset = 0;
int scale_left = 1, scale_right = 1;

TimerLED timer_led(PIN_LED, true);
bool led_state;
EncButton <EB_TICK, PIN_BTN> btn;
CalibrationServer server(servo, PIN_SERVO, [](uint8_t _l, uint8_t _c, uint8_t _r) {
  EEPROM.begin(EEPROM_SIZE);
  if (!_l && !_c && !_r) {
    EEPROM.put(0, 0);
    Serial.print(F("[EEPROM] Boundaries cleared from EEPROM"));
  } else {
    EEPROM.put(0, 42);
    EEPROM.put(1, _l);
    EEPROM.put(2, _c);
    EEPROM.put(3, _r);
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
    server.setBoundaries(_l, _c, _r);
    Serial.print(F("[EEPROM] Servo calibration restored: "));
    Serial.print((int)_l);
    Serial.print(F("; "));
    Serial.print((int)_c);
    Serial.print(F("; "));
    Serial.println((int)_r);
  } else {
    Serial.println(F("[EEPROM] StartByte is not 42. No calibration restored"));
  }
  EEPROM.end();
}

void StartCalibration() {
  server.start();
}

void changeTask(const int& _task) {
  task = _task;
  switch (task) {
    case 0:
      timer_led.setIntervals(2925, 25);
      // Off
      break;
    case 1:
      timer_led.setIntervals(725, 25);
      // External signals
      break;
    case 2:
      timer_led.setIntervals(100, 250, 1600, 50);
      // Internal schedule
      break;
    case 3:
      timer_led.setIntervals(50, 50);
      StartCalibration();
      break;
    default:
      break;
  }
  timer_led.restart();
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

  changeTask(3);

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
    case 0:  // Off
      break;
    case 1: // External signals
      break;
    case 2: // Internal schedule
      break;
    default: // Calibration
      server.listen();
      break;
  }
}