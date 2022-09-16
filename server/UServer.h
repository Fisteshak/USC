#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <string>
#include <memory>


class UServer
{

public:
    //конструктор с указанием
    //порта
    //IP
    //максимального кол-ва соединенией (опционально)
    UServer(int listenerPort, std::string listenerIP, uint32_t max_connections = 100);
    //деструктор
    ~UServer();
    //Инициализирует сетевой интерфейс для сокетов.
    //Возвращает true в случае успеха, false в случае неудачи.
    bool initWinsock();
    // создает сокет-слушатель    
    SOCKET createListener(); 
    //закрывает интерфейсы winsock
    void cleanupWinsock();  
    //запускает сервер (цикл приема данных) 
    void run();             

private:        
    int max_connections;
    int listenerPort;
    std::string listenerIP;    
};


