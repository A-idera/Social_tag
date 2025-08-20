// RadioModule.h (Slave Version - Updated)

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

class RadioModule {
public:
    void begin(SPIClass& spi, uint8_t ce, uint8_t csn);
    void powerUp();

    // 【修改】: 監聽字串封包
    bool listenForCommand(String& command);
    
    void sendJoinRequest(uint8_t deviceId);
    void sendResponse(const String& response);
    void sendTestPacket(uint8_t deviceId);

private:
    RF24* _radio;
};