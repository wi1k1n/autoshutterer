#ifndef EXTERNALSIGNAL_H
#define EXTERNALSIGNAL_H

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "Servo.h"

#include "credentials.h"

class ExternalServer {
private:
    ESP8266WebServer* _server;
    Servo* _servo;
    int _servo_pin;
    uint8_t _left, _center, _right;
    uint16_t _delay;
    bool isMoving;
    unsigned long int moveFinishTime;
public:
    ExternalServer(Servo &servo, const int servo_pin, const uint16_t delay = 500, const uint16_t port = 80) {
        _server = new ESP8266WebServer(port);
        _servo = &servo;
        _servo_pin = servo_pin;
        _delay = delay;
        
        _server->on("/", [this]() {
            _server->send(200, "text/plain", "wrong query");
        });
        _server->on("/left", [this]() {
            if (!authorize()) return;
            if (isMoving) return _server->send(200, "text/plain", "try again later");
            MoveLeft(_delay);
            _server->send(200, "text/plain", "success");
        });
        _server->on("/right", [this]() {
            if (!authorize()) return;
            if (isMoving) return _server->send(200, "text/plain", "try again later");
            MoveRight(_delay);
            _server->send(200, "text/plain", "success");
        });
        _server->onNotFound([this]() {
            if (!authorize()) return;
            _server->send(404, "text/plain", "not found");
        });
    }
    ~ExternalServer() {
        delete _server;
    }
    void start() {
        _servo->attach(_servo_pin);
        _servo->write(_center);
        _server->begin();
        Serial.println(F("[External] Webserver started"));
    }
    void stop() {
        _servo->detach();
        _server->stop();
        Serial.println(F("[External] Webserver stopped"));
    }
    void listen() {
        if (isMoving && millis() >= moveFinishTime) {
            _servo->write(_center);
            Serial.print("[Servo] Center started at");
            Serial.println(millis());
            isMoving = false;
        }
        _server->handleClient();
    }
    void setBoundaries(const uint8_t _l, const uint8_t _c, const uint8_t _r) {
        _left = _l;
        _center = _c;
        _right = _r;
    }
    void MoveLeft(const uint16_t _delay = 200) {
        _servo->write(_left);
        Serial.print("[Servo] Left started at ");
        Serial.println(millis());
        isMoving = true;
        moveFinishTime = millis() + _delay;
    }
    void MoveRight(const uint16_t _delay = 200) {
        _servo->write(_right);
        Serial.print("[Servo] Right started at ");
        Serial.println(millis());
        isMoving = true;
        moveFinishTime = millis() + _delay;
    }
private:
    bool authorize() {
        if (!_server->authenticate(WEB_LOGIN, WEB_PASSWORD)) {
            _server->requestAuthentication(DIGEST_AUTH);
            return false;
        }
        return true;
    }
};

#endif /* EXTERNALSIGNAL_H */