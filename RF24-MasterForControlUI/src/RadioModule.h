// RadioModule.h (Master Version - Updated)

#pragma once
#include <Arduino.h>
#include <RF24.h>
#include <vector>

class RadioModule {
public:
    RadioModule();
    void begin(uint8_t ce, uint8_t csn);
    void powerUp();

    // 【更新】: 使用新的封包格式廣播指令
    bool broadcastPacket(const String& packet, int burst_count = 5);
    
    // 發現模式相關函式
    void switchToDiscoveryMode();
    bool listenForDiscovery(uint8_t& deviceId);

    // 操作模式相關函式
    void switchToOperationMode(const std::vector<uint8_t>& devices);
    bool listenForResponse(String& payload, uint8_t& pipeNum);

    void flush();

private:
    RF24 _radio;
};