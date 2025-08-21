// MasterLedModule.h

#pragma once
#include <Arduino.h>

class MasterLedModule {
public:
    MasterLedModule();
    void begin();
    void startDiscoveryMode();
    void stopDiscoveryMode();
    void setIdleMode();
    void update(); // RGB loop update function
    
private:
    void setRgbColor(uint8_t red, uint8_t green, uint8_t blue);
    void turnOff();
    
    // RGB loop state management
    bool _rgb_loop_active;
    unsigned long _last_rgb_change_time;
    uint8_t _current_rgb_state; // 0=Red, 1=Green, 2=Blue
    static const unsigned long RGB_CHANGE_INTERVAL_MS = 2000; // 2 seconds
};
