// RadioModule.cpp (Master Final Version - Updated)

#include "RadioModule.h"
#include "Config.h"

RadioModule::RadioModule() : _radio(NRF_CE, NRF_CSN) {}

void RadioModule::begin(uint8_t ce, uint8_t csn) {
    if (!_radio.begin()) {
        Serial.println("FATAL: NRF24L01 not responding!");
        while (1);
    }
    _radio.setPALevel(RF24_PA_LOW);
    _radio.setDataRate(RF24_1MBPS);
    _radio.setChannel(76);
    _radio.setCRCLength(RF24_CRC_16);
    _radio.setAutoAck(true);
    _radio.setRetries(15, 15);
}

void RadioModule::powerUp() {
    _radio.powerUp();
}

// 【新增】: 廣播字串封包的函式
bool RadioModule::broadcastPacket(const String& packet, int burst_count) {
    _radio.stopListening();
    _radio.openWritingPipe(command_pipe);
    bool success = false;
    char buffer[32];
    packet.toCharArray(buffer, sizeof(buffer));

    for (int i = 0; i < burst_count; ++i) {
        if (_radio.write(&buffer, sizeof(buffer))) {
            success = true;
        }
        delay(5);
    }
    _radio.startListening();
    return success;
}

void RadioModule::switchToDiscoveryMode() {
    _radio.stopListening();
    _radio.openReadingPipe(1, discovery_pipe);
    _radio.flush_rx();
    _radio.flush_tx();
    _radio.startListening();
    Serial.println("[Radio] Switched to Discovery Mode. Listening on DISC1.");
}

bool RadioModule::listenForDiscovery(uint8_t& deviceId) {
    if (_radio.available()) {
        _radio.read(&deviceId, sizeof(deviceId));
        return true;
    }
    return false;
}

void RadioModule::switchToOperationMode(const std::vector<uint8_t>& devices) {
    _radio.stopListening();
    Serial.print("[Radio] Switched to Operation Mode. Listening on pipes: ");
    for (size_t i = 0; i < devices.size(); ++i) {
        if (i < 5) { // RF24 library supports up to 6 pipes (0-5), pipe 0 is for writing.
            char pipe_str[6];
            sprintf(pipe_str, "%dNODE", devices[i]);
            _radio.openReadingPipe(i + 1, (const uint8_t*)pipe_str);
            Serial.print(pipe_str); Serial.print(" ");
        }
    }
    Serial.println();
    _radio.startListening();
}

bool RadioModule::listenForResponse(String& payload, uint8_t& pipeNum) {
    if (_radio.available(&pipeNum)) {
        char buffer[32] = "";
        _radio.read(&buffer, sizeof(buffer));
        payload = String(buffer);
        return true;
    }
    return false;
}

void RadioModule::flush() {
    _radio.flush_rx();
    _radio.flush_tx();
}