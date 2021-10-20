/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
  Arduino IDE example: Examples > Arduino OTA > BasicOTA.ino
  Sourse: https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
*********/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Servo.h"
#include <GyverButton.h>
#include <TimerLED.h>

#include "credentials.h"
#include "calibrator.h"

// Replace with your network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int PIN_LED = D4;
const int PIN_SERVO = D6;
const int PIN_BTN = D5;

Servo servo;
int task;  // 0: off, 1: external signals, 2: internal schedule, 3: calibration
int offset = 0;
int scale_left = 1, scale_right = 1;

TimerLED timer_led(PIN_LED, true);
bool led_state;
GButton btn(PIN_BTN);

void configureWiFi();
void configureArduinoOTA();
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  configureWiFi();
  configureArduinoOTA();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  servo.attach(PIN_SERVO);
  changeTask(1);
}

void loop() {
  ArduinoOTA.handle();
  btn.tick();
  timer_led.tick();

  if (btn.isClick()) {
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
//      calibration();
      break;
  }
}


void changeTask(const int& _task) {
  task = _task;
  switch (task) {
    case 0: timer_led.setIntervals(2925, 25); break;
    case 1: timer_led.setIntervals(725, 25); break;
    case 2: timer_led.setIntervals(100, 250, 1600, 50); break;
    case 3: timer_led.setIntervals(50, 50); break;
    default: break;
  }
  timer_led.restart();
  Serial.print("Task changed to #");
  Serial.println(task);
}


void calibration() {
  if (server.status() == CLOSED) {
    server.begin();
    Serial.println("WiFi server started");
  }
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("new client"));
  client.setTimeout(1000);

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  int val;
  if (req.indexOf(F("/gpio/0")) != -1) {
    val = 0;
  } else if (req.indexOf(F("/gpio/1")) != -1) {
    val = 1;
  } else {
    Serial.println(F("invalid request"));
  }
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now "));
  client.print((val) ? F("high") : F("low"));
  client.print(F("<br><br>Click <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/1'>here</a> to switch LED GPIO on, or <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/0'>here</a> to switch LED GPIO off.</html>"));

  Serial.println(F("Disconnecting from client"));
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
void configureArduinoOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("ArduinoOTA - start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nArduinoOTA - end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("ArduinoOTA - Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("ArduinoOTA - Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
