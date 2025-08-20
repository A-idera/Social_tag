// IGameMode.h

#pragma once
#include <vector>
#include <map>
#include "RadioModule.h"

class IGameMode {
public:
    virtual ~IGameMode() {}

    virtual void start(RadioModule& radio, std::vector<uint8_t>& slaves) = 0;
    virtual void update() = 0;

    /**
     * @brief 【已修改】: 回傳型別改為 String，以便將要發送的指令回傳給主程式
     * @param payload 收到的封包內容
     * @param sender_id 發送者的ID
     * @return 需要主程式代為廣播的指令字串，如果不需要則回傳空字串
     */
    virtual String handlePacket(const String& payload, uint8_t sender_id) = 0;

    virtual bool isFinished() = 0;
};