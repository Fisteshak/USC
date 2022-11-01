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

    //присоединиться к серверу и начать принимать данные
    status connectTo(std::string IP, uint32_t port);
    //отсоединиться от сервера
    void disconnect();
    //цикл приема данных
    void recvHandlingLoop();
    void joinThreads();
    status getStatus();

    //типы буферов данных
    using DataBuffer = std::vector <char>;
    using DataBufferStr = std::string;

    //тип обработчика данных
    using DataHandler = std::function <void(DataBuffer&)>;
    //тип обработчика принятия соединения
    using ConnHandler = std::function <void()>;

    //установить размер посылка
    void set_block_size(uint32_t size);
    //установить обработчик получения данных
    void setDataHandler(DataHandler handler);
    //установить обработчик принятия соединения
    void setConnHandler(ConnHandler handler);
    //установить обработчик отключения соединения
    void setDisconnHandler(ConnHandler handler);
    //отправить данные всем клиентам

    //обработчик получений данных
    DataHandler dataHandler;
    //обработчик принятия нового соединения
    ConnHandler connHandler;
    //обработчик отключения соединения
    ConnHandler disconnHandler;



private:

    bool initWinsock();  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.
    uint32_t block_size = 1024;
    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    SOCKET clientSocket;

};

