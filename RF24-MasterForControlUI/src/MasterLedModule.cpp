// MasterLedModule.cpp

#include "MasterLedModule.h"
#include "Config.h"

MasterLedModule::MasterLedModule() {
    _rgb_loop_active = false;
    _last_rgb_change_time = 0;
    _current_rgb_state = 0; // Start with red
}

void MasterLedModule::begin() {
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    turnOff();
}

void MasterLedModule::startDiscoveryMode() {
    Serial.println("[RGB] Starting Discovery Mode RGB loop");
    _rgb_loop_active = true;
    _current_rgb_state = 0;
    _last_rgb_change_time = millis();
    setRgbColor(255, 0, 0); // Start with red
}

void MasterLedModule::stopDiscoveryMode() {
    Serial.println("[RGB] Stopping Discovery Mode RGB loop");
    _rgb_loop_active = false;
    turnOff();
}

void MasterLedModule::setIdleMode() {
    _rgb_loop_active = false;
    turnOff();
}

void MasterLedModule::update() {
    if (!_rgb_loop_active) return;
    
    if (millis() - _last_rgb_change_time >= RGB_CHANGE_INTERVAL_MS) {
        _current_rgb_state = (_current_rgb_state + 1) % 3;
        _last_rgb_change_time = millis();
        
        switch (_current_rgb_state) {
            case 0: // Red
                setRgbColor(255, 0, 0);
                Serial.println("[RGB] Switching to RED");
                break;
            case 1: // Green  
                setRgbColor(0, 255, 0);
                Serial.println("[RGB] Switching to GREEN");
                break;
            case 2: // Blue
                setRgbColor(0, 0, 255);
                Serial.println("[RGB] Switching to BLUE");
                break;
        }
    }
}

void MasterLedModule::setRgbColor(uint8_t red, uint8_t green, uint8_t blue) {
    // RGB simulation using two LEDs with PWM
    // LED1_PIN: Controls red and green channels
    // LED2_PIN: Controls blue channel
    
    if (red > 0) {
        // Red color: LED1 on, LED2 off
        analogWrite(LED1_PIN, red);
        analogWrite(LED2_PIN, 0);
    } else if (green > 0) {
        // Green color: LED1 dimmer, LED2 off
        analogWrite(LED1_PIN, green);
        analogWrite(LED2_PIN, 0);
    } else if (blue > 0) {
        // Blue color: LED1 off, LED2 on
        analogWrite(LED1_PIN, 0);
        analogWrite(LED2_PIN, blue);
    } else {
        // All off
        analogWrite(LED1_PIN, 0);
        analogWrite(LED2_PIN, 0);
    }
}

void MasterLedModule::turnOff() {
    analogWrite(LED1_PIN, 0);
    analogWrite(LED2_PIN, 0);
}
