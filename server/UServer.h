#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <string>
#include <memory>

constexpr int max_connections = 100;
class UServer
{

public:
    UServer(int listenerPort, std::string listenerIP);
    ~UServer();
    bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.
    SOCKET createListener(); // создает сокет-слушатель    

private:        
    struct pollfd fds[max_connections];
    int listenerPort;
    std::string listenerIP;    
};


