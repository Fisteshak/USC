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
#include <any>

class UServer
{

public:
    //тип буфера для данных
    using DataBuffer = std::vector <char>;
    using DataBufferStr = std::string;
    class client;

private:

    //тип обработчика данных
    using data_handler_t = std::function <void(DataBuffer&, client&)>;
    //тип обработчика принятия соединения

    using conn_handler_t = std::function <void(client&)>;

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
    UServer(int listenerPort, std::string listenerIP, uint32_t nMaxConnections = 100);
    //деструктор
    ~UServer();

    //запускает сервер (цикл приема данных)
    status run();
    //останавливает сервер
    void stop();
    //получить размер посылки
    uint32_t get_block_size();
    //установить размер посылка
    void set_block_size(uint32_t size);
    //установить обработчик получения данных
    void set_data_handler(data_handler_t handler);
    //установить обработчик принятия соединения
    void set_conn_handler(conn_handler_t handler);
    //установить обработчик отключения соединения
    void set_disconn_handler(conn_handler_t handler);
    //отправить данные всем клиентам
    void sendData(DataBuffer& data);
    void sendData(DataBufferStr& data);
    //отправить данные выбранному клиенту
    void sendDataTo(DataBuffer& data, client& cl);
    //получить статус
    status getStatus();
    //получить порт
    uint32_t getPort();

private:

    //завершить все потоки
    void joinThreads();
    //Инициализирует сетевой интерфейс для сокетов.
    //Возвращает true в случае успеха, false в случае неудачи.
    bool initWinsock();
    // создает сокет-слушатель
    SOCKET createListener();
    //закрывает интерфейсы winsock
    void cleanupWinsock();
    //цикл приема данных
    void handlingLoop();
    //закрывает и уничтожает массивы соединений
    void cleanup();
    //максимальное количество соединений (включая сокет listener)
    uint32_t nMaxConnections;
    //количество текущих подключенных соединений (включая сокет listener)
    uint32_t nConnections = 0;
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

public:
    // bool operator==(const UServer::client &clientb) {
    //      return (fd == clientb.fd);
    // }
    bool operator==(const client& lhs)
    {
        return (lhs.fd == fd);
    }

    bool operator!=(const UServer::client &clientb) {
         return (fd != clientb.fd);
    }


    enum status : uint8_t {
        connected = 1,
        disconnected = 2,
        error_send_data = 3
    };

    client& operator= (const client &clientb)
    {
        // делаем копию
        _status = clientb._status;
        ref = clientb.ref;
        fd = clientb.fd;

        // возвращаем существующий объект, чтобы
        // можно было включить этот оператор в цепочку
        return *this;
    }

    status getStatus();
    SOCKET getSocket();
    //void* ref;
    std::any ref;
    // client();
    ~client();
    UServer::client::status sendData(DataBuffer& data);
    UServer::client::status sendData(DataBufferStr& data);

    void disconnect();
private:
    //дескриптор сокета
    SOCKET fd;
    //ссылка на пользовательский обьект
    //статус
    client::status _status = client::status::disconnected;
};

