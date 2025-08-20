// Config.h (Master Version - Updated for Modularity)

#pragma once
#include <Arduino.h>

// --- 1. 硬體接腳定義 ---
#define NRF_CE       (4)
#define NRF_CSN      (5)
// --- LED制御ピン ---
#define LED1_PIN (26)        // LED1用ピン（起動時制約なし）
#define LED2_PIN (27)        // LED2用ピン
#define LED_COUNT 7
#define STATUS_LED_PIN LED1_PIN

// --- マトリックス表示接続ピン ---
// HSPI（セカンダリSPI）を使用してVSPIとの競合を回避
#define MATRIX_CLK_PIN   (14)    // HSPI_CLK (GPIO14は安全な汎用ピン)
#define MATRIX_DATA_PIN  (13)    // HSPI_MOSI (GPIO13は安全な汎用ピン)
#define MATRIX_CS_PIN    (25)    // HSPI_CS (GPIO25は安全な汎用ピン)
#define MATRIX_DEVICES   (8)     // 8×8モジュール × 8 = 8×64マトリックス

// --- 2. NRF24L01 通訊設定 ---
const uint8_t command_pipe[6] = "CMD01";
const uint8_t discovery_pipe[6] = "DISC1";



// --- 3. 狀態管理 ---
enum SystemMode { 
    MODE_DISCOVERY,         // 發現模式
    MODE_IDLE,              // 閒置模式
    MODE_GAME_RUNNING       // 【新增】: 遊戲執行中狀態
    // MODE_WAIT_FOR_READER 和 MODE_WAIT_FOR_ACK 已被移入遊戲模組內部管理
};

// --- 4. 常數設定 ---
// RESPONSE_TIMEOUT_MS 已移入遊戲模組內部管理
const int MIN_DEVICES_REQUIRED = 2;