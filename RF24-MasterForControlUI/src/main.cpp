/**
 * @file main.cpp
 * @brief NRF Master Controller
 * @details Master controller for RF24 communication with slaves.
 */
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <map>
#include "Config.h"
#include "RadioModule.h"
#include "IGameMode.h"


// --- Global Objects & State ---
std::map<uint8_t, unsigned long> slave_last_heartbeat;
std::map<uint8_t, String> id_to_name;
RadioModule radio;
SystemMode current_mode = MODE_DISCOVERY;
std::vector<uint8_t> discovered_slaves;
IGameMode* currentGameMode = nullptr;

 

unsigned long lastTimeoutCheckTime = 0;
const unsigned long SLAVE_TIMEOUT_MS = 30000;
const unsigned long TIMEOUT_CHECK_INTERVAL_MS = 5000;
 
// --- Function Declarations ---
void handleDiscoveryState();
void processSerialCommand(const String& command);
void checkSlaveTimeouts();
void handleRadioPacket(const String& payload, uint8_t pipeNum);
void switchToIdleMode();
void switchToDiscoveryMode();
void removeSlave(uint8_t id_to_remove);

// Auto discovery scheduling
static bool auto_discovery_started = false;
static unsigned long auto_discovery_start_time = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n------- NRF Master Controller --------");
    pinMode(STATUS_LED_PIN, OUTPUT);
    radio.begin(NRF_CE, NRF_CSN);
    randomSeed(analogRead(A0));
    // Defer discovery start by 10 seconds per requirement
    Serial.println("[SYSTEM] Entering DISCOVERY mode for 10 seconds...");
    auto_discovery_start_time = millis() + 10000UL;
}
 
void loop() {
    // Start discovery after the scheduled delay
    if (!auto_discovery_started && millis() >= auto_discovery_start_time) {
        Serial.println("Radio switched to Discovery mode.");
        switchToDiscoveryMode();
        auto_discovery_started = true;
    }

    if (current_mode != MODE_DISCOVERY) {
        checkSlaveTimeouts();
    }

    String payload;
    uint8_t pipeNum;
    if (radio.listenForResponse(payload, pipeNum)) {
        handleRadioPacket(payload, pipeNum);
    }
 
    if (Serial.available() > 0) {
        String command_str = Serial.readStringUntil('\n');
        command_str.trim();

        if (command_str.length() > 0) {
            if (command_str.startsWith("*") && command_str.endsWith("#")) {
               processSerialCommand(command_str);
            } 
            else if (command_str.startsWith("RMV:")) {
                removeSlave(command_str.substring(4).toInt());
            }
        }
    }
 
    switch (current_mode) {
        case MODE_DISCOVERY:
            handleDiscoveryState();
            break;
        case MODE_GAME_RUNNING:
            if (currentGameMode) {
                currentGameMode->update();
                if (currentGameMode->isFinished()) {
                    radio.broadcastPacket("*STOP_ALL#");
                    delay(100);
                    switchToIdleMode();
                }
            }
            break;
        case MODE_IDLE:
            break;
    }
}

void handleRadioPacket(const String& payload, uint8_t pipeNum) {
    if (pipeNum <= 0 || pipeNum > discovered_slaves.size()) return;
    uint8_t sender_id = discovered_slaves[pipeNum - 1];
    if (payload.startsWith("*HEARTBEAT_")) {
        slave_last_heartbeat[sender_id] = millis();
        String name = id_to_name.count(sender_id) ? id_to_name[sender_id] : String("");
        const char* mode_str = (current_mode == MODE_DISCOVERY) ? "MODE_DISCOVERY" : (current_mode == MODE_IDLE) ? "MODE_IDLE" : "MODE_GAME_RUNNING";
        Serial.printf("[debug] HB from ID=%d name=%s state=%s\n", sender_id, name.c_str(), mode_str);
        return; 
    }
    Serial.printf(">>> [Radio RX] From #%d: [%s]\n", sender_id, payload.c_str());
    switch(current_mode) {
        case MODE_IDLE:
            if (payload.startsWith("Channel Test")) Serial.printf("   ↳ [PIPE TEST] OK!\n");
            else Serial.printf("   ↳ [IDLE] Unsolicited packet.\n");
            break;
        case MODE_GAME_RUNNING:
            if (currentGameMode) {
                String command_to_send = currentGameMode->handlePacket(payload, sender_id);
                if (command_to_send.length() > 0) {
                    Serial.printf("   ↳ [Game Logic] Broadcasting command: %s\n", command_to_send.c_str());
                    radio.broadcastPacket(command_to_send, 3);
                }
            }
            break;
        default: break;
    }
}

void handleDiscoveryState() {
    uint8_t new_device_id = 0;
    if (radio.listenForDiscovery(new_device_id)) {
        if (new_device_id == 0) {
            Serial.println("[WARNING] Ignored a discovery request from invalid Device ID 0 (likely noise).");
            return;
        }
        bool is_already_known = (std::find(discovered_slaves.begin(), discovered_slaves.end(), new_device_id) != discovered_slaves.end());
        if (!is_already_known) {
            discovered_slaves.push_back(new_device_id);
            std::sort(discovered_slaves.begin(), discovered_slaves.end());
            Serial.printf("[DISCOVERY] New slave joined! ID: %d. Total: %d\n", new_device_id, discovered_slaves.size());
        } else {
            Serial.printf("[DISCOVERY] Known slave #%d re-confirmed its presence.\n", new_device_id);
        }
        slave_last_heartbeat[new_device_id] = millis();
        char ack_cmd[2] = {(char)new_device_id, 0};
        radio.broadcastPacket(String(ack_cmd), 3);
        // 受信後は対象IDへ STOP を送って5秒点灯→消灯させ、IDLEへ
        String stop_cmd = "*STOP_" + String(new_device_id) + "#";
        radio.broadcastPacket(stop_cmd, 3);
    }
}

void processSerialCommand(const String& command) {
    Serial.printf("[COMMAND] Received: %s\n", command.c_str());
    else if (command == "*STOP_ALL#") {
        Serial.println("====== EMERGENCY STOP received from UI! ======");
        radio.broadcastPacket("*STOP_ALL#");
        delay(100); 
        switchToIdleMode();
    } 
    else if (command.startsWith("*NAME_")) {
        int eq_pos = command.indexOf('=');
        int hash_pos = command.lastIndexOf('#');
        if (eq_pos > 0 && hash_pos > eq_pos) {
            String id_part = command.substring(6, eq_pos);
            String name_part = command.substring(eq_pos + 1, hash_pos);
            uint8_t id = (uint8_t)id_part.toInt();
            id_to_name[id] = name_part;
            Serial.printf("[debug] Register name: id=%u, name=%s\n", id, name_part.c_str());
            // NAME_ 自体をフォワード（子機側で自身の名前ログに使う）
            radio.broadcastPacket(command, 3);
            String blink_cmd = "*BLINK_WHITE_" + id_part + "#";
            radio.broadcastPacket(blink_cmd, 3);
        }
    }
    else if (command.startsWith("*REMOVEJOIN_")) {
        Serial.printf("[SYSTEM] Broadcasting kick command: %s\n", command.c_str());
        radio.broadcastPacket(command);
    }
    else if (command == "*DISCOVERY_01#") {
        switchToDiscoveryMode();
    } 
    else if (command == "*DISCOVERY_00#") {
        Serial.println("Ending Discovery. Switching to Idle Mode.");
        switchToIdleMode();
        if (!discovered_slaves.empty()) {
            Serial.println("[SYSTEM] Broadcasting pipe test command to all slaves...");
            radio.broadcastPacket("*TESTPIPE_ALL#");
        }
    } 
    else if (command == "*SCENARIO1_START#") {
        Serial.println("[INFO] Scenario 1 not implemented yet.");
    }
    else if (command == "*SCENARIO2_START#") {
        Serial.println("[INFO] Scenario 2 not implemented yet.");
    }
}

void removeSlave(uint8_t id_to_remove) {
    auto it = std::remove(discovered_slaves.begin(), discovered_slaves.end(), id_to_remove);
    if (it != discovered_slaves.end()) {
        discovered_slaves.erase(it, discovered_slaves.end());
        slave_last_heartbeat.erase(id_to_remove);
        Serial.printf("[SYSTEM] Device #%d removed. Total devices: %d\n", id_to_remove, discovered_slaves.size());
        if(current_mode == MODE_GAME_RUNNING) {
            Serial.println("[SYSTEM] A device disconnected during the game. Returning to Idle.");
            switchToIdleMode(); 
        } else if (current_mode == MODE_IDLE) {
            Serial.println("[SYSTEM] Device list changed while idle. Re-configuring radio pipes.");
            radio.switchToOperationMode(discovered_slaves);
        }
    }
}
void checkSlaveTimeouts() {
    if (millis() - lastTimeoutCheckTime > TIMEOUT_CHECK_INTERVAL_MS) {
        std::vector<uint8_t> timed_out_slaves;
        for (auto const& [id, last_seen] : slave_last_heartbeat) {
            if (millis() - last_seen > SLAVE_TIMEOUT_MS) {
                String name = id_to_name.count(id) ? id_to_name[id] : String("");
                Serial.printf("[debug] WARN missing heartbeat >30s: ID=%u name=%s\n", id, name.c_str());
                timed_out_slaves.push_back(id);
            }
        }
        for (uint8_t id : timed_out_slaves) {
            // 仕様変更: タイムアウト時はログのみ残し、接続は維持する
            Serial.printf("[TIMEOUT] Slave #%d has timed out. Keeping connection (no removal).\n", id);
        }
        lastTimeoutCheckTime = millis();
    }
}
void switchToIdleMode() {
    Serial.println("\n[STATUS] System switching to Idle Mode.");
    if (currentGameMode) {
        delete currentGameMode;
        currentGameMode = nullptr;
    }
    if (discovered_slaves.size() < MIN_DEVICES_REQUIRED) {
        Serial.printf("Not enough devices. (%d/%d found). Returning to Discovery Mode.\n", discovered_slaves.size(), MIN_DEVICES_REQUIRED);
        switchToDiscoveryMode();
    } else {
        Serial.printf("Ready for UI commands. %d devices are online.\n", discovered_slaves.size());
        current_mode = MODE_IDLE;
        radio.switchToOperationMode(discovered_slaves);
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}
void switchToDiscoveryMode() {
    Serial.println("\n[STATUS] System now in Discovery Mode. Waiting for slaves to join...");
    if (currentGameMode) {
        delete currentGameMode;
        currentGameMode = nullptr;
    }
    current_mode = MODE_DISCOVERY;
    radio.switchToDiscoveryMode();
    digitalWrite(STATUS_LED_PIN, HIGH);
}