#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

class UServer
{

public:
    enum status : uint8_t {
        up = 0,
        stopped = 1,
        error_listener_create = 2,
        error_accept_connection = 3,
        error_winsock_init = 5,
        error_wsapoll = 6,
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
    void handlingLoop();
    //запускает сервер (цикл приема данных) 
    status run();
    //останавливает сервер
    void stop(); 
    //завершить все потоки
    void joinThreads();    
    uint32_t get_block_size();
    void set_block_size(uint32_t size);
    
private:        
    int max_connections;
    //Port слушателя (сервера)
    int listenerPort;                  
    //IP слушателя (сервера) 
    std::string listenerIP;            
    //статус сервера
    std::atomic <status> _status = status::stopped;
    //потое обработки данных
    std::thread handlingLoopThread;     
    //массив дескрипторов сокетов
	std::vector <struct pollfd> fds;    
    //сокет-слушатель
    SOCKET listener;    
    //                
    uint32_t block_size;
    //
    std::function <void()> data_handler;
    std::function <void()> conn_handler;



};

void aa();


