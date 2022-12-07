#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

using Socket = SOCKET;

class UClient
{
public:
    enum status : uint8_t {
        disconnected = 1,
        connected,
        paused,
        error_socket_connect,
        error_socket_create,
        error_send_data,
        error_recv_data
    };

    UClient();
    ~UClient();

    void pause();

    //присоединиться к серверу и начать принимать данные
    status connectTo(const std::string& IP, const uint32_t port);
    //отсоединиться от сервера
    void disconnect();

    status getStatus();

    //типы буферов данных
    using DataBuffer = std::vector <char>;
    using DataBufferStr = std::string;

    //тип обработчика данных
    using DataHandler = std::function <void(DataBuffer&)>;
    //тип обработчика принятия соединения
    using ConnHandler = std::function <void()>;

    //установить размер посылки
    void setBlockSize(const uint32_t size);
    uint32_t getBlockSize();
    //установить обработчик получения данных
    void setDataHandler(const DataHandler handler);
    //установить обработчик принятия соединения
    void setConnHandler(const ConnHandler handler);
    //установить обработчик отключения соединения
    void setDisconnHandler(const ConnHandler handler);


    status sendData(const DataBuffer& data);
    status sendData(const DataBufferStr& data);

    int getLastError();

private:
    //Инициализирует сетевой интерфейс для сокетов.
    //Возвращает true в случае успеха, false в случае неудачи.
    bool initWinsock();

    //цикл приема данных
    void recvHandlingLoop();
    void joinThreads();

    //получить len байт из массива data на сокет sock
    // ! len должен быть меньше либо равен размеру массива data
    //возвращает количество отосланных байт, 0 при отсоединении клиента, -1 или ошибке
    int recvAll(const Socket sock, char* data, const int len);


    int sendAll(const char* data, const int len);

    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvData(DataBuffer& data);
    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvData(DataBufferStr& data);

    //обработчик получений данных
    DataHandler dataHandler;
    //обработчик принятия нового соединения
    ConnHandler connHandler;
    //обработчик отключения соединения
    ConnHandler disconnHandler;

    uint32_t blockSize = 1024;
    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    SOCKET clientSocket;

};
