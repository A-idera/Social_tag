// RadioModule.cpp (Slave Version - Updated)

#include "RadioModule.h"
#include "Config.h"

// The 'begin' function and others not shown here are assumed to be the same as your original slave file.
// If you need the full file, let me know. The key changes are in listenForCommand and sendResponse.

void RadioModule::begin(SPIClass& spi, uint8_t ce, uint8_t csn) {
    // This assumes you are using the SPI library and a pointer to the radio object
    // as in your original structure.
    _radio = new RF24(ce, csn);
    if (!_radio->begin(&spi)) {
        Serial.println(F("Radio hardware not responding!!"));
        while (1) {} // Hold in infinite loop
    }
    _radio->setPALevel(RF24_PA_LOW);
    _radio->setDataRate(RF24_1MBPS);
    _radio->setChannel(76);
    _radio->setCRCLength(RF24_CRC_16);
    _radio->setAutoAck(true);
    _radio->openReadingPipe(1, command_pipe); // Listen on the common command pipe
    _radio->flush_rx();
    _radio->flush_tx();
    _radio->startListening();
}

void RadioModule::powerUp() {
    _radio->powerUp();
}

// 【修改】: 監聽並讀取完整的字串封包
bool RadioModule::listenForCommand(String& command) {
    if (_radio->available()) {
        char buffer[32] = "";
        _radio->read(&buffer, sizeof(buffer));
        command = String(buffer);
        // 清理可能存在的空字元
        command.trim(); 
        if (command.length() > 0) {
            return true;
        }
    }
    return false;
}

void RadioModule::sendJoinRequest(uint8_t deviceId) {
    _radio->stopListening();
    _radio->openWritingPipe(discovery_pipe);
    _radio->write(&deviceId, sizeof(deviceId));
    _radio->startListening();
}

// 【修改】: 發送字串格式的回應
void RadioModule::sendResponse(const String& response) {
    _radio->stopListening();
    char pipe_name[6];
    sprintf(pipe_name, "%dNODE", DEVICE_ID);
    _radio->openWritingPipe((const uint8_t*)pipe_name);

    char buffer[32];
    response.toCharArray(buffer, sizeof(buffer));
    _radio->write(&buffer, sizeof(buffer));

    _radio->startListening();
}

void RadioModule::sendTestPacket(uint8_t deviceId) {
    _radio->stopListening();
    char pipe_name[6];
    sprintf(pipe_name, "%dNODE", deviceId);
    _radio->openWritingPipe((const uint8_t*)pipe_name);

    char text[32];
    sprintf(text, "Channel Test from #%d", deviceId);
    _radio->write(&text, sizeof(text));

    _radio->startListening();
}