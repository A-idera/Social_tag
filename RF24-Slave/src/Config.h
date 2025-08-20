// Config.h

#pragma once
#include <Arduino.h>

// --- 1. 選擇設備 ID (數字 ID) ---
#ifndef DEVICE_ID
    #define DEVICE_ID 1 // 預設為從機 1
#endif

// --- 2. 硬體接腳定義 ---
// 【HSPI for PN532】
#define PN532_SCK  (26)
#define PN532_MISO (25)
#define PN532_MOSI (33)
#define PN532_SS   (32)

// 【VSPI for NRF24L01】
#define NRF_SCK  (18)
#define NRF_MISO (19)
#define NRF_MOSI (23)
#define NRF_CSN  (5)
#define NRF_CE   (4)

// WS2812B (FastLED) 燈條設定
#define LED_STRIP_PIN  (13)
#define LED_COUNT      (9)

#define SCORE_MODE_BUTTON1_PIN (27) // 設為讀卡機按鈕
#define SCORE_MODE_BUTTON2_PIN (14) // 取消讀卡按鈕

// --- 3. NRF 通訊位址 ---
const uint8_t command_pipe[6] = "CMD01";    // 主機 -> 從機 的通用指令頻道
const uint8_t discovery_pipe[6] = "DISC1";  // 從機 -> 主機 的專用發現/加入頻道

// --- 4. NFC 模擬器設定 ---
const char* const NDEF_BASE_URL = "https://socialtag.io/user/";

// UID 仍然根據 DEVICE_ID 不同，以便物理上區分
#if DEVICE_ID == 1
    const uint8_t my_uid[3] = { 0x01, 0x01, 0x01 };
#elif DEVICE_ID == 2
    const uint8_t my_uid[3] = { 0x02, 0x02, 0x02 };
#elif DEVICE_ID == 3
    const uint8_t my_uid[3] = { 0x03, 0x03, 0x03 };
#elif DEVICE_ID == 4
    const uint8_t my_uid[3] = { 0x04, 0x04, 0x04 };
#else // 其他 ID 的預設值
    const uint8_t my_uid[3] = { 0xDE, 0xAD, 0xBE };
#endif


// --- 5. 系統狀態與超時 ---
/**
 * @brief 【已修改】: 新增 MODE_SPOTLIGHT 狀態
 */
enum SystemMode {
    MODE_JOINING,          // 加入模式 (開機後的初始狀態)
    MODE_CONFIRM_BLINKING, // 已加入確認模式 (閃爍紅燈的短暫狀態)
    MODE_CHANNEL_TEST,
    MODE_IDLE,             // 閒置模式
    MODE_READER,           // 讀卡機模式
    MODE_EMULATOR,         // 模擬卡模式
    MODE_TEAM_WAITING,     // 組隊中，等待下一階段模式 (黃燈)
    MODE_SPOTLIGHT,         // 團隊抽籤中，被選中模式 (彩虹)
    MODE_SCORE_EMULATOR,   // 積分模式下的預設狀態 (模擬卡)
    MODE_SCORE_READER,     // 積分模式下，按下按鈕1後的狀態 (讀卡機)
    MODE_COOLDOWN          // 積分模式下的冷卻狀態
};

const unsigned long TASK_TIMEOUT_MS = 20000;