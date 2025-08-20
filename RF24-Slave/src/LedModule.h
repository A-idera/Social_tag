// LedModule.h

#pragma once
#include <FastLED.h>

class LedModule {
public:
    LedModule(uint16_t num_pixels, int8_t pin);
    void begin();
    void showJoiningMode();
    void showSolidRed();
    void showReaderMode();
    void showEmulatorMode();
    void showWaitingMode();
    void showSpotlightMode(); // 【新增】: 宣告顯示彩虹抽籤燈的函式
    void blinkWhiteForSeconds(uint16_t seconds);
    void turnOff();

private:
    uint16_t _num_pixels;
    CRGB* _leds;
    uint8_t _rainbow_hue;
};