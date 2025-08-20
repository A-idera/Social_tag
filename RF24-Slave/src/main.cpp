/**
 * @file main.cpp
 * @brief NRF Slave Device (v3.1 - Logic & Parser Fix)
 * @details Fixes command prefix mismatch for Score Mode and corrects the main state machine logic.
 */
#include <Arduino.h>
#include <SPI.h>
#include <EasyButton.h> 
#include "Config.h"
#include "RadioModule.h"
#include "NfcModule.h"
#include "LedModule.h"

// --- Global Objects & State ---
SPIClass hspi(HSPI);
SPIClass vspi(VSPI);
RadioModule radio;
NfcModule nfc;
LedModule leds(LED_COUNT, LED_STRIP_PIN);
SystemMode current_mode = MODE_JOINING;
TaskHandle_t nrf_task_handle;
TaskHandle_t main_logic_task_handle;
volatile bool stop_signal = false;
volatile bool blink_white_pending = false;
String registered_name = "";

EasyButton button1(SCORE_MODE_BUTTON1_PIN);
EasyButton button2(SCORE_MODE_BUTTON2_PIN);
volatile bool button1_pressed = false;
volatile bool button2_pressed = false;

// --- Function Declarations ---
void nrf_task(void* pvParameters);
void main_logic_task(void* pvParameters);
void resetToIdleState();
bool parseAndCheckId(const String& command, const String& prefix);
void handleButton1Press();
void handleButton2Press();
void setupButtons();

void setup() {
    Serial.begin(115200);
    leds.begin();
    hspi.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    vspi.begin(NRF_SCK, NRF_MISO, NRF_MOSI, NRF_CSN);
    radio.begin(vspi, NRF_CE, NRF_CSN);
    if (!nfc.begin(hspi, PN532_SS)) {
        Serial.println("[WARN] PN532 not found. Continuing discovery/join anyway.");
    }
    setupButtons();
    Serial.printf("\n--- Slave Device #%d Booted Up ---\n", DEVICE_ID);
    xTaskCreatePinnedToCore(nrf_task, "NRF_Task", 4096, NULL, 1, &nrf_task_handle, 0);
    xTaskCreatePinnedToCore(main_logic_task, "MainLogic_Task", 4096, NULL, 1, &main_logic_task_handle, 1);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}

void handleButton1Press() { button1_pressed = true; }
void handleButton2Press() { button2_pressed = true; }
void setupButtons() {
    button1.begin();
    button2.begin();
    button1.onPressed(handleButton1Press);
    button2.onPressed(handleButton2Press);
}

bool parseAndCheckId(const String& command, const String& prefix) {
    if (!command.startsWith(prefix)) return false;
    int start_index = prefix.length();
    int end_index = command.length() - 1;
    String ids_str = command.substring(start_index, end_index);
    if (ids_str == "ALL") return true;
    String my_id_str = String(DEVICE_ID);
    int current_pos = 0;
    while(current_pos < ids_str.length()){
        int separator_pos = ids_str.indexOf('-', current_pos);
        if(separator_pos == -1) separator_pos = ids_str.length();
        String id_part = ids_str.substring(current_pos, separator_pos);
        if(id_part == my_id_str) return true;
        current_pos = separator_pos + 1;
    }
    return false;
}

void nrf_task(void* pvParameters) {
    radio.powerUp();
    for (;;) {
        String received_packet;
        if (radio.listenForCommand(received_packet)) {
            Serial.printf("[NRF] Received packet: %s\n", received_packet.c_str());
            
            // --- 【關鍵修正】移除所有指令前綴中多餘的結尾底線 '_' ---
            if (parseAndCheckId(received_packet, "*READ_")) {
                current_mode = MODE_READER;
            } else if (parseAndCheckId(received_packet, "*EMULATE_")) {
                current_mode = MODE_EMULATOR;
            } else if (parseAndCheckId(received_packet, "*TESTPIPE_")) {
                current_mode = MODE_CHANNEL_TEST;
            } else if (parseAndCheckId(received_packet, "*SETCOLOR_YELLOW_")) {
                current_mode = MODE_TEAM_WAITING;
            } else if (parseAndCheckId(received_packet, "*SETCOLOR_RAINBOW_")) {
                current_mode = MODE_SPOTLIGHT;
            } else if (parseAndCheckId(received_packet, "*SCORE_MODE_START")) { // 修正
                current_mode = MODE_SCORE_EMULATOR;
            } else if (parseAndCheckId(received_packet, "*SCORE_MODE_STOP")) { // 修正
                resetToIdleState();
            } else if (parseAndCheckId(received_packet, "*COOLDOWN_START_")) {
                current_mode = MODE_COOLDOWN;
            } else if (parseAndCheckId(received_packet, "*COOLDOWN_END_")) {
                if (current_mode == MODE_COOLDOWN) {
                    current_mode = MODE_SCORE_EMULATOR;
                }
            }

            if(current_mode == MODE_JOINING && received_packet.length() > 0 && received_packet[0] == DEVICE_ID) {
                current_mode = MODE_CONFIRM_BLINKING;
            }
            
            if (parseAndCheckId(received_packet, "*STOP_")) {
                stop_signal = true;
            } else if (parseAndCheckId(received_packet, "*REMOVEJOIN_")) {
                Serial.println("[SYSTEM] Kicked by master! Returning to JOINING mode.");
                current_mode = MODE_JOINING;
            } else if (parseAndCheckId(received_packet, "*BLINK_WHITE_")) {
                blink_white_pending = true;
            } else if (received_packet.startsWith("*NAME_")) {
                int eq_pos = received_packet.indexOf('=');
                int hash_pos = received_packet.lastIndexOf('#');
                if (eq_pos > 0 && hash_pos > eq_pos) {
                    String id_part = received_packet.substring(6, eq_pos);
                    String name_part = received_packet.substring(eq_pos + 1, hash_pos);
                    if (id_part == String(DEVICE_ID)) {
                        registered_name = name_part;
                        Serial.printf("[debug] Stored name for ID=%d: %s\n", DEVICE_ID, registered_name.c_str());
                    }
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void main_logic_task(void* pvParameters) {
    unsigned long lastHeartbeatSendTime = 0;
    const unsigned long HEARTBEAT_INTERVAL_MS = 5000;

    for (;;) {
        button1.read();
        button2.read();

        if (button1_pressed) {
            button1_pressed = false;
            if (current_mode == MODE_SCORE_EMULATOR) {
                current_mode = MODE_SCORE_READER;
            }
        }
        if (button2_pressed) {
            button2_pressed = false;
            if (current_mode == MODE_SCORE_READER) {
                Serial.println("[SYSTEM] Reader mode cancelled by Button 2.");
                current_mode = MODE_SCORE_EMULATOR;
            }
        }

        if (stop_signal) {
            Serial.println("[debug] STOP received; lighting RED for 5s then turning off");
            leds.showSolidRed();
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            leds.turnOff();
            resetToIdleState();
            stop_signal = false;
            continue;
        }

        if (blink_white_pending) {
            Serial.printf("[debug] ID=%d color=WHITE name=%s (blink ~5s)\n", DEVICE_ID, registered_name.c_str());
            leds.blinkWhiteForSeconds(5);
            resetToIdleState();
            blink_white_pending = false;
            continue;
        }

        if (current_mode != MODE_JOINING && current_mode != MODE_CONFIRM_BLINKING) {
            if (millis() - lastHeartbeatSendTime > HEARTBEAT_INTERVAL_MS) {
                radio.sendResponse("*HEARTBEAT_" + String(DEVICE_ID) + "#");
                lastHeartbeatSendTime = millis();
            }
        }

        // --- 【關鍵修正】重新組織 switch 結構，確保所有 case 都被正確處理 ---
        switch (current_mode) {
            case MODE_JOINING:
                // 起動直後は赤点灯しJOIN送信（debugログ）
                Serial.printf("[debug] ID=%d LED=RED sending JOIN\n", DEVICE_ID);
                leds.showSolidRed();
                radio.sendJoinRequest(DEVICE_ID);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                break;
            case MODE_CONFIRM_BLINKING:
                Serial.println("[STATE] ==> Entering Confirm Blinking.");
                for(int i=0; i<5; i++){
                    leds.showReaderMode(); vTaskDelay(200 / portTICK_PERIOD_MS);
                    leds.turnOff(); vTaskDelay(200 / portTICK_PERIOD_MS);
                }
                resetToIdleState();
                break;
            case MODE_CHANNEL_TEST: {
                Serial.println("[STATE] ==> Performing Commanded Channel Test.");
                radio.sendTestPacket(DEVICE_ID);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                resetToIdleState();
                break;
            }
            case MODE_IDLE:
                leds.turnOff();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
            case MODE_TEAM_WAITING:
                leds.showWaitingMode();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
            case MODE_SPOTLIGHT:
                leds.showSpotlightMode();
                vTaskDelay(20 / portTICK_PERIOD_MS);
                break;
            case MODE_READER: { // 舊的 1v1 或 Team Building 的 Reader 模式
                Serial.println("[STATE] ==> Entering Reader Mode (Standard).");
                leds.showReaderMode();
                String result = "";
                bool success = false;
                unsigned long reader_start_time = millis();
                while (millis() - reader_start_time < TASK_TIMEOUT_MS) {
                    if (stop_signal) break;
                    if (nfc.runReaderTask(result)) { success = true; break; }
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                if(!stop_signal) radio.sendResponse(success ? ("Read " + result) : "Reader Timed Out");
                resetToIdleState();
                break;
            }
            case MODE_EMULATOR: { // 舊的 1v1 或 Team Building 的 Emulator 模式
                Serial.println("[STATE] ==> Entering Emulator Mode (Standard).");
                leds.showEmulatorMode();
                nfc.initEmulator(DEVICE_ID);
                unsigned long start_time = millis();
                while (millis() - start_time < TASK_TIMEOUT_MS) {
                    if (stop_signal) break;
                    nfc.emulateOneTick();
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                nfc.releaseEmulator();
                if (stop_signal) radio.sendResponse("Emulator Stopped ACK");
                resetToIdleState();
                break;
            }
            
            // --- 積分模式的狀態處理 ---
            case MODE_SCORE_EMULATOR:
                leds.showEmulatorMode(); // 綠燈，等待被感應
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
            
            case MODE_SCORE_READER: {
                Serial.println("[STATE] ==> Entering Score Reader Mode.");
                leds.showReaderMode(); // 紅燈，主動讀卡
                String result = "";
                bool success = false;
                unsigned long reader_start_time = millis();
                
                while (millis() - reader_start_time < TASK_TIMEOUT_MS) {
                    if (stop_signal) break;
                    button2.read();
                    if (button2_pressed) {
                        button2_pressed = false;
                        break; 
                    }
                    if (nfc.runReaderTask(result)) {
                        success = true;
                        break;
                    }
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }

                if (success) {
                    radio.sendResponse("Read " + result);
                }
                
                current_mode = MODE_SCORE_EMULATOR; // 自動回到模擬卡狀態
                break;
            }

            case MODE_COOLDOWN:
                leds.showWaitingMode(); // 黃燈，表示冷卻中
                vTaskDelay(100 / portTICK_PERIOD_MS);
                break;
        }
    }
}

void resetToIdleState() {
    if (current_mode != MODE_IDLE) {
        Serial.println("[STATE] ==> Resetting to IDLE state.");
    }
    stop_signal = false;
    button1_pressed = false; 
    button2_pressed = false;
    current_mode = MODE_IDLE;
    leds.turnOff();
}