#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <string>
#include <memory>
#include <atomic>


class UServer
{

public:
    enum status : uint8_t {
        up = 0,
        stopped = 1,
        error_listener_create = 2,
        error_accept_connection = 3,
        error_winsock_init = 5,
        error = 4
    };

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
    //цикл приема данных
    status handlingLoop();
    //запускает сервер (цикл приема данных) 
    status run();  
    //останавливает сервер
    void stop(); 


    
private:        
    int max_connections;               
    int listenerPort;                  //Port слушателя (сервера) 
    std::string listenerIP;            //IP слушателя (сервера) 
    std::atomic <status> _status = status::stopped;   //статус сервера
};


