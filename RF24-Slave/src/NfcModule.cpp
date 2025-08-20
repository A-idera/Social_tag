#include "NfcModule.h"
#include "Config.h"
#include "NdefMessage.h"

// begin() 函式保持不變
bool NfcModule::begin(SPIClass& spi, uint8_t ss) {
    PN532_SPI* pn532_spi_interface = new PN532_SPI(spi, ss);
    _nfc = new PN532(*pn532_spi_interface);
    _emulator = new EmulateTag(*pn532_spi_interface);
    _nfcAdapter = new NfcAdapter(*pn532_spi_interface); 
    _nfcAdapter->begin();
    uint32_t versiondata = _nfc->getFirmwareVersion();
    if (!versiondata) return false;
    return true;
}

/**
 * @brief 【最終策略更新】: 恢復為一個簡單且極度可靠的 UID 讀取器。
 * 此函式現在只會讀取標籤的 UID 並回報。所有複雜的 NDEF 解析都已移除。
 */
bool NfcModule::runReaderTask(String& result) {
    uint8_t uid[7] = {0};
    uint8_t uidLength;

    // 1. 偵測標籤並讀取其 UID
    if (_nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
        
        // 2. 將 UID 格式化為一個標準的字串 (例如 "UID:08020202")
        String uid_str = "UID:";
        for (uint8_t i = 0; i < uidLength; i++) {
            if(uid[i] < 0x10) uid_str += '0'; // 如果需要，補上前面的 0
            uid_str += String(uid[i], HEX);
        }
        uid_str.toUpperCase(); // 確保十六進位字母為大寫，方便主機解析
        result = uid_str;
        return true;
    }
    
    return false;
}

// initEmulator() 函式保持不變。它仍然會模擬 NDEF，但這不影響讀卡機只讀取 UID
void NfcModule::initEmulator(uint8_t deviceId) {
    NdefMessage message = NdefMessage();
    String full_url = String(NDEF_BASE_URL) + String(deviceId);
    Serial.printf("[NFC EMU] Emulating NDEF URL: %s\n", full_url.c_str());
    message.addUriRecord(full_url);
    int messageSize = message.getEncodedSize();
    if (messageSize > 120) {
        Serial.println("[NFC EMU] Error: NDEF message too large!");
        return;
    }
    uint8_t ndefBuf[messageSize];
    message.encode(ndefBuf);
    _emulator->setNdefFile(ndefBuf, messageSize);
    _emulator->setUid((uint8_t*)my_uid);
    _emulator->init();
}

// emulateOneTick() 和 releaseEmulator() 函式保持不變
void NfcModule::emulateOneTick() {
    _emulator->emulate(50);
}

void NfcModule::releaseEmulator() {
    _nfc->inRelease();
}