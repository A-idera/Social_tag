// MasterMatrixModule.h

#pragma once
#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>

class MasterMatrixModule {
public:
    MasterMatrixModule();
    void begin();
    void showWelcomeMessage();
    void showIdleDisplay();
    void clear();
    void update(); // For animation updates
    
private:
    MD_Parola* _parola;
    bool _welcome_displayed;
    
    // Matrix display settings
    static const uint8_t HARDWARE_TYPE = MD_MAX72XX::FC16_HW;
    static const uint8_t MAX_DEVICES = MATRIX_DEVICES;
};
