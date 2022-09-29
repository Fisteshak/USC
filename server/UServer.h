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
    //тип буфера для данных
    //typedef std::vector <char> data_buffer_t;    
    using data_buffer_t = std::vector <char>;        

private:
    //тип обработчика данных
    using data_handler_t = std::function <void(data_buffer_t&)>;
    //тип обработчика принятия соединения
    using conn_handler_t = std::function <void()>;

    class client;

public:    

    enum status : uint8_t {
        up = 0,
        stopped = 1,
        connected = 7,
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
    
    //установить обработчик получения данных
    void set_data_handler(data_handler_t handler);

    //установить обработчик принятия соединения
    void set_conn_handler(conn_handler_t handler);

    //установить обработчик отключения соединения
    void set_disconn_handler(conn_handler_t handler);

    
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
    //каждый элемент соответствует элементу clients с тем же индексом
	std::vector <struct pollfd> fds;   
    //массив клиентов
    //каждый элемент соответствует элементу fds с тем же индексом
    std::vector <client> clients;
    //сокет-слушатель
    SOCKET listener;    
    //размер блока при получении данных
    uint32_t block_size = 1024;
    //обработчик получений данных
    data_handler_t data_handler;
    //обработчик принятия нового соединения
    conn_handler_t conn_handler;
    //обработчик отключения соединения
    conn_handler_t disconn_handler;

};

class UServer::client
{
    friend class UServer;
private:
    //дескриптор сокета
    SOCKET fd;

public:
    enum status : uint8_t {
        connected = 1,
        disconnected = 2
    };
    status _status = status::disconnected;

    client();
    ~client();    
    void sendData(data_buffer_t& data);
    void disconnect();
};