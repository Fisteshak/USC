#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <any>
#include <string>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

using Socket = SOCKET;

#ifndef SOCKET_ERROR
    #define SOCKET_ERROR -1
#endif

class UServer
{

public:
    //тип буфера для данных
    using DataBuffer = std::vector <unsigned char>;
    using DataBufferStr = std::string;
    class Client;

private:

    //тип обработчика данных
    using DataHandler = std::function <void(DataBuffer&, Client&)>;
    //тип обработчика принятия соединения
    using ConnHandler = std::function <void(Client&)>;

public:

    enum status : uint8_t {
        up,
        stopped,
        error_listener_create,
        error_accept_connection,
        error_winsock_init,
        error_wsapoll,
        error
    };

    //конструктор с указанием
    //IP
    //порта
    //максимального кол-ва соединенией (опционально)
    UServer(std::string listenerIP, int listenerPort, uint32_t nMaxConnections = 100);
    UServer() {};
    //деструктор
    ~UServer();

    //запускает сервер (цикл приема данных)
    status run();
    //останавливает сервер
    void stop();
    //получить размер посылки
    uint32_t getBlockSize();
    //установить размер посылка
    void setBlockSize(const uint32_t size);
    //установить обработчик получения данных
    void setDataHandler(const DataHandler handler);
    //установить обработчик принятия соединения
    void setConnHandler(const ConnHandler handler);
    //установить обработчик отключения соединения
    void setDisconnHandler(const ConnHandler handler);
    //отправить данные всем клиентам
    void sendPacket(const DataBuffer& data);
    void sendPacket(const DataBufferStr& data);
    //получить статус
    status getStatus();
    //получить порт
    uint32_t getPort();
    //установить IP
    //IP не будет изменен, если сервер работает в данный момент времени (getStatus() == status::up)
    //возвращает true при успехе, false при неудаче
    bool setIP(const std::string& IP);
    //установить порт
    //порт не будет изменен, если сервер работает в данный момент времени (getStatus() == status::up)
    //возвращает true при успехе, false при неудаче
    bool setPort(const uint32_t& port);

    //массив клиентов
    //каждый элемент соответствует элементу fds с тем же индексом
    std::vector <Client> clients;
    //количество текущих подключенных соединений (включая сокет listener)
    uint32_t nConnections = 0;

private:

    //завершить все потоки
    void joinThreads();
    //Инициализирует сетевой интерфейс для сокетов.
    //Возвращает true в случае успеха, false в случае неудачи.
    bool initWinsock();
    // создает сокет-слушатель
    Socket createListener();
    //закрывает интерфейсы winsock
    void cleanupWinsock();
    //цикл приема данных
    void handlingLoop();
    //закрыть соедиение под указанным номером
    void closeConnection(const uint32_t connectionNum);
    //добавить соединение в массив соединенией
    //возвращает номер соединения в массиве fds/clients
    uint32_t addConnection(const Socket newConneection);

    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(const Client& sock, DataBuffer& data);
    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(const Client& sock, DataBufferStr& data);
    //закрывает и уничтожает массивы соединений fds/clients
    void cleanup();
    //отправляет len байт из массива data на сокет fd
    // ! len должен быть меньше либо равен размеру массива data
    //при ошибке: возвращает SOCKET_ERROR
    //при успехе: возвращает количество отправленных байт
    static int sendAll(const Socket fd, const char *data, const int len);

    //получить len байт в массив data из сокета sock
    // ! len должен быть меньше либо равен размеру массива data
    //при отсоединении клиента sock: возвращает 0
    //при ошибке: возвращает SOCKET_ERROR
    //при отсоединении: возвращает 0
    //при успехе: возвращает количество полученных байт
    int recvAll(const Socket sock, char* data, const int len);
    //максимальное количество соединений (включая сокет listener)
    uint32_t nMaxConnections;
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
    //сокет-слушатель
    Socket listener;
    //размер блока при получении данных
    uint32_t block_size = 1024;
    //обработчик получений данных
    DataHandler dataHandler;
    //обработчик принятия нового соединения
    ConnHandler connHandler;
    //обработчик отключения соединения
    ConnHandler disconnHandler;

};

class UServer::Client
{
friend class UServer;

public:


    bool operator==(const UServer::Client &clienta) {
        return (clienta.fd == fd);
    }

    bool operator!=(const UServer::Client &clienta) {
        return (clienta.fd != fd);
    }


    enum status : uint8_t {
        connected = 1,
        disconnected = 2,
        error_send_data = 3
    };

    Client& operator= (const Client &clientb)
    {
        // делаем копию
        _status = clientb._status;
        ref = clientb.ref;
        fd = clientb.fd;

        // возвращаем существующий объект, чтобы
        // можно было включить этот оператор в цепочку
        return *this;
    }

    //сервер, который владеет (управляет) этим клиентом
    UServer* owner;

    status getStatus();
    Socket getSocket();
    //пользовательская переменная
    std::any ref;
    // client();

    ~Client();
    //отправить пакет данных
    UServer::Client::status sendPacket(const DataBuffer& data);
    //отправить пакет данных
    UServer::Client::status sendPacket(const DataBufferStr& data);

    //получить IP клиента в виде строки
    std::string getIPstr();
    //получить IP клиента в виде числа
    uint32_t getIP();


    void disconnect();
private:
    //дескриптор сокета
    Socket fd;
    //статус
    Client::status _status = Client::status::disconnected;
};
