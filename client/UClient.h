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
        connected = 2,
        paused = 5,
        error_socket_connect = 3,
        error_socket_create = 4
    };

    UClient();
    ~UClient();

    void pause();
    status connectTo(std::string IP, uint32_t port);
    void disconnect();
    void recvHandlingLoop();
    void joinThreads();
    status getStatus();
private:

    bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.

    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    SOCKET clientSocket;

};

