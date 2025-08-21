// MasterMatrixModule.cpp

#include "MasterMatrixModule.h"
#include "Config.h"

MasterMatrixModule::MasterMatrixModule() {
    _welcome_displayed = false;
    _parola = nullptr;
}

void MasterMatrixModule::begin() {
    // Initialize the MD_Parola library with HSPI pins
    _parola = new MD_Parola(HARDWARE_TYPE, MATRIX_DATA_PIN, MATRIX_CLK_PIN, MATRIX_CS_PIN, MAX_DEVICES);
    
    if (_parola->begin()) {
        Serial.println("[MATRIX] Matrix display initialized successfully");
        _parola->setIntensity(5); // Set brightness (0-15)
        clear();
    } else {
        Serial.println("[MATRIX] ERROR: Failed to initialize matrix display");
    }
}

void MasterMatrixModule::showWelcomeMessage() {
    if (_parola && !_welcome_displayed) {
        Serial.println("[MATRIX] Displaying 'welcome!!' message");
        _parola->displayClear();
        _parola->displayText("welcome!!", PA_CENTER, 60, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        _welcome_displayed = true;
    }
}

void MasterMatrixModule::showIdleDisplay() {
    if (_parola) {
        Serial.println("[MATRIX] Switching to idle display");
        _parola->displayClear();
        _parola->displayText("READY", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
        _welcome_displayed = false;
    }
}

void MasterMatrixModule::clear() {
    if (_parola) {
        _parola->displayClear();
        _welcome_displayed = false;
    }
}

void MasterMatrixModule::update() {
    if (_parola) {
        _parola->displayAnimate();
    }
}
