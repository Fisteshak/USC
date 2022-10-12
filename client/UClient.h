#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <cstdint>
#include <thread>
#include <atomic>

class UClient
{
public:
    enum status : uint8_t {
        disconnected = 1,
        connected = 2
    };

    UClient();
    ~UClient();

private:
    bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.
    status connectTo(std::string IP, uint32_t port);
    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    
};

