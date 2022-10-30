#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

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
    using DataBuffer = std::vector <char>;
    using DataBufferStr = std::string;

    using conn_handler_t = std::function <void()>;
    using data_handler_t = std::function <void(DataBuffer&)>;

    void set_data_handler(data_handler_t handler);
    //установить обработчик принятия соединения
    void set_conn_handler(conn_handler_t handler);
    //установить обработчик отключения соединения

    //обработчик получений данных
    data_handler_t data_handler;
    //обработчик принятия нового соединения
    conn_handler_t conn_handler;
    //обработчик отключения соединения
    conn_handler_t disconn_handler;

private:

    bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.
    uint32_t block_size = 1024;
    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    SOCKET clientSocket;

};

