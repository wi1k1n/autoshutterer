#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "Servo.h"

class CalibrationServer {
private:
    ESP8266WebServer* _server;
    Servo* _servo;
    uint8_t _pos{90};
    int _servo_pin;
    uint8_t _left{85}, _center{90}, _right{95};
    bool _initialized;
    std::function<void(uint8_t, uint8_t, uint8_t)> _store;
public:
    CalibrationServer(Servo &servo, const int servo_pin, const std::function<void(uint8_t, uint8_t, uint8_t)>& storeFunc, const uint16_t port = 80) {
        _server = new ESP8266WebServer(port);
        _servo = &servo;
        _servo_pin = servo_pin;
        _store = storeFunc;

        _server->on("/", [this]() {
            if (!authorize()) return;
            sendMainPage();
        });
        _server->on("/cc", [this]() { // Center
            if (!authorize()) return;
            _pos = 90;
            setServo(_pos);
            sendMainPage(F("Set to center"));
        });
        _server->on("/ls", [this]() { // <
            if (!authorize()) return;
            if (_pos > 0)
                _pos--;
            setServo(_pos);
            sendMainPage(F("Tiny left"));
        });
        _server->on("/ll", [this]() { // <<<
            if (!authorize()) return;
            if (_pos >= 5)
                _pos -= 5;
            else
                _pos = 0;
            setServo(_pos);
            sendMainPage(F("Step left"));
        });
        _server->on("/rs", [this]() { // >
            if (!authorize()) return;
            if (_pos < 180)
                _pos++;
            setServo(_pos);
            sendMainPage(F("Tiny right"));
        });
        _server->on("/rl", [this]() { // >>>
            if (!authorize()) return;
            if (_pos <= 175)
                _pos += 5;
            else
                _pos = 180;
            setServo(_pos);
            sendMainPage(F("Step right"));
        });
        _server->on("/sl", [this]() { // Save left
            if (!authorize()) return;
            _left = _pos;
            _initialized = true;
            sendMainPage(F("Saved left"));
        });
        _server->on("/sc", [this]() { // Save center
            if (!authorize()) return;
            _center = _pos;
            _initialized = true;
            sendMainPage(F("Saved center"));
        });
        _server->on("/sr", [this]() { // Save right
            if (!authorize()) return;
            _right = _pos;
            _initialized = true;
            sendMainPage(F("Saved right"));
        });
        _server->on("/st", [this]() { // Store in EEPROM
            if (!authorize()) return;
            _store(_left, _center, _right);
            sendMainPage(F("Stored in EEPROM"));
        });
        _server->on("/dl", [this]() { // Clear EEPROM
            if (!authorize()) return;
            _store(0, 0, 0);
            _left = _center = _right = 0;
            _initialized = false;
            sendMainPage(F("EEPROM cleared"));
        });
        _server->on("/gl", [this]() { // Set left
            if (!authorize()) return;
            setServo(_left);
            sendMainPage(F("Set to left"));
        });
        _server->on("/gc", [this]() { // Set center
            if (!authorize()) return;
            setServo(_center);
            sendMainPage(F("Set to center"));
        });
        _server->on("/gr", [this]() { // Set right
            if (!authorize()) return;
            setServo(_right);
            sendMainPage(F("Set to right"));
        });
        _server->onNotFound([this]() {
            if (!authorize()) return;
            _server->send(404, "text/plain", "not found");
        });
    }
    ~CalibrationServer() {
        delete _server;
    }
    void start() {
        _servo->attach(_servo_pin);
        _server->begin();
        Serial.println(F("[Calibration] Webserver started"));
    };
    void stop() {
        _servo->detach();
        _server->stop();
        Serial.println(F("[Calibration] Webserver stopped"));
    }
    void listen() {
        _server->handleClient();
    }
    void setBoundaries(const uint8_t _l, const uint8_t _c, const uint8_t _r) {
        _left = _l;
        _center = _c;
        _right = _r;
        _initialized = true;
    }
private:
    void setServo(uint8_t val) {
        _servo->write(val);
    }
    bool authorize() {
        if (!_server->authenticate(WEB_LOGIN, WEB_PASSWORD)) {
            _server->requestAuthentication(DIGEST_AUTH);
            return false;
        }
        return true;
    }
    void sendMainPage(const String& msg = "") {
        String content = "";
        content += F("<!DOCTYPE HTML><html><head>");
        content += F("<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\">");
        content += F("</head><body><div class=\"d-flex flex-column justify-content-md-center align-items-center vh-100\"><h2>");
        content += _pos;
        content += F("</h2><div>");
        content += F("<a href=\"/\" class=\"btn btn-lg btn-success m-1\"> Main </a>");
        content += F("</div><div>");
        content += F("<a href=\"ll\" class=\"btn btn-lg btn-primary m-1\"> <<< </a>");
        content += F("<a href=\"ls\" class=\"btn btn-lg btn-primary m-1\"> < </a>");
        content += F("<a href=\"cc\" class=\"btn btn-lg btn-info m-1\"> Center </a>");
        content += F("<a href=\"rs\" class=\"btn btn-lg btn-primary m-1\"> > </a>");
        content += F("<a href=\"rl\" class=\"btn btn-lg btn-primary m-1\"> >>> </a>");
        content += F("</div><h5>");
        content += _left;
        content += "; ";
        content += _center;
        content += "; ";
        content += _right;
        content += F("</h5>");
        if (_initialized) {
            content += F("<div><a href=\"gl\" class=\"btn btn-lg btn-secondary m-1\"> Set left </a>");
            content += F("<a href=\"gc\" class=\"btn btn-lg btn-secondary m-1\"> Set center </a>");
            content += F("<a href=\"gr\" class=\"btn btn-lg btn-secondary m-1\"> Set right </a></div>");
        }
        content += F("<div>");
        content += F("<a href=\"sl\" class=\"btn btn-lg btn-warning m-1\"> Save left </a>");
        content += F("<a href=\"sc\" class=\"btn btn-lg btn-warning m-1\"> Save center </a>");
        content += F("<a href=\"sr\" class=\"btn btn-lg btn-warning m-1\"> Save right </a>");
        content += F("</div><div>");
        content += F("<a href=\"st\" class=\"btn btn-lg btn-success m-1\"> Store in EEPROM </a>");
        content += F("<a href=\"dl\" class=\"btn btn-lg btn-danger m-1\"> Clear EEPROM </a>");
        content += F("</div>");
        if (msg.length())
            content += F("<p>(") + msg + F(")</p>");
        content += F("</div></body></html>");
        _server->send(200, "text/html", content);
    }
};

#endif /* CALIBRATION_H */