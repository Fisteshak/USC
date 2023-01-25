#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

using Socket = SOCKET;

#ifndef SOCKET_ERROR
    #define SOCKET_ERROR -1
#endif

class UClient
{
public:
    const static uint8_t CRYPTO_ENABLED = 0x1;

    enum status : uint8_t {
        disconnected = 1,
        connected,
        paused,
        error_socket_connect,
        error_socket_create,
        error_send_data,
        error_recv_data
    };

    UClient(uint16_t flags = 0x0);
    ~UClient();

    //void pause();

    //присоединиться к серверу и начать принимать данные
    status connectTo(const std::string& IP, const uint32_t port);
    //отсоединиться от сервера
    void disconnect();

    status getStatus();

    //типы буферов данных
    using DataBuffer = std::vector <unsigned char>;
    using DataBufferStr = std::string;

    //тип обработчика данных
    using DataHandler = std::function <void(DataBuffer&)>;
    //тип обработчика принятия соединения
    using ConnHandler = std::function <void()>;

    using tbyte = uint8_t;
    using uint = unsigned int;


    //установить обработчик получения данных
    void setDataHandler(const DataHandler handler);
    //установить обработчик принятия соединения
    void setConnHandler(const ConnHandler handler);
    //установить обработчик отключения соединения
    void setDisconnHandler(const ConnHandler handler);

    status sendPacket(const DataBuffer& data);
    status sendPacket(const DataBufferStr& data);

    int getLastError();

private:
    //Инициализирует сетевой интерфейс для сокетов.
    //Возвращает true в случае успеха, false в случае неудачи.
    bool initWinsock();

    //цикл приема данных
    void recvHandlingLoop();
    void joinThreads();

    //отправляет len байт из массива data на сокет fd
    // ! len должен быть меньше либо равен размеру массива data
    //при ошибке: возвращает SOCKET_ERROR
    //при успехе: возвращает количество отправленных байт
    int sendAll(const char* data, const int len);

    //получить len байт в массив data
    // ! len должен быть меньше либо равен размеру массива data
    //при отсоединении клиента sock: возвращает 0
    //при ошибке: возвращает SOCKET_ERROR
    //при отсоединении: возвращает 0
    //при успехе: возвращает количество полученных байт
    int recvAll(char* data, const int len);

    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(DataBuffer& data);
    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(DataBufferStr& data);

    //обработчик получений данных
    DataHandler dataHandler;
    //обработчик принятия нового соединения
    ConnHandler connHandler;
    //обработчик отключения соединения
    ConnHandler disconnHandler;

    uint32_t blockSize = 1024;
    std::atomic <status> _status = status::disconnected;
    std::thread recvHandlingLoopThread;
    Socket clientSocket;

    // crypto

    bool cryptoEnabled = false;
    uint32_t AESKeyLength = 16;
    uint32_t RSAKeyLength = 4096;
    std::vector<tbyte> AESKey;


};
