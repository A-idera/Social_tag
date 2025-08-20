#pragma once
#include <Arduino.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <emulatetag.h>
#include <NfcAdapter.h> // 【新增】: 引入 NfcAdapter 標頭檔

class NfcModule {
public:
    bool begin(SPIClass& spi, uint8_t ss); 
    bool runReaderTask(String& result);
    
    void emulateOneTick(); 
    void initEmulator(uint8_t deviceId);
    void releaseEmulator();

private:
    PN532* _nfc;
    EmulateTag* _emulator;
    NfcAdapter* _nfcAdapter; // 【新增】: 用於高階 NDEF 讀取的物件
};