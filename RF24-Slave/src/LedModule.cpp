// LedModule.cpp

#include "LedModule.h"
#include "Config.h"

LedModule::LedModule(uint16_t num_pixels, int8_t pin) {
    _num_pixels = num_pixels;
    _leds = new CRGB[_num_pixels];
    _rainbow_hue = 0;
    FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(_leds, _num_pixels);
}

void LedModule::begin() {
    FastLED.setBrightness(50);
    turnOff();
}

void LedModule::showJoiningMode() {
    fill_rainbow(_leds, _num_pixels, _rainbow_hue++, 7);
    FastLED.show();
}

void LedModule::showSolidRed() {
    fill_solid(_leds, _num_pixels, CRGB::Red);
    FastLED.show();
}

void LedModule::showReaderMode() {
    fill_solid(_leds, _num_pixels, CRGB::Red); 
    FastLED.show();
}

void LedModule::showEmulatorMode() {
    fill_solid(_leds, _num_pixels, CRGB::Green);
    FastLED.show();
}

void LedModule::showWaitingMode() {
    fill_solid(_leds, _num_pixels, CRGB::Yellow);
    FastLED.show();
}

/**
 * @brief 【新增】: 顯示彩虹抽籤燈 (效果同 Joining)
 */
void LedModule::showSpotlightMode() {
    fill_rainbow(_leds, _num_pixels, _rainbow_hue++, 7);
    FastLED.show();
}

void LedModule::blinkWhiteForSeconds(uint16_t seconds) {
    unsigned long end_time = millis() + (unsigned long)seconds * 1000UL;
    bool on = false;
    while (millis() < end_time) {
        if (on) {
            fill_solid(_leds, _num_pixels, CRGB::White);
        } else {
            fill_solid(_leds, _num_pixels, CRGB::Black);
        }
        FastLED.show();
        on = !on;
        delay(250);
    }
    turnOff();
}


void LedModule::turnOff() {
    fill_solid(_leds, _num_pixels, CRGB::Black);
    FastLED.show();
    pinMode(LED_STRIP_PIN, OUTPUT);
    digitalWrite(LED_STRIP_PIN, LOW);
}