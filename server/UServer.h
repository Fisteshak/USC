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

typedef SOCKET Socket;

class UServer
{

public:
    //тип буфера для данных
    using DataBuffer = std::vector <char>;
    using DataBufferStr = std::string;
    class Client;

private:

    //тип обработчика данных
    using DataHandler = std::function <void(DataBuffer&, Client&)>;
    //тип обработчика принятия соединения
    using ConnHandler = std::function <void(Client&)>;

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
    //IP
    //порта
    //максимального кол-ва соединенией (опционально)
    UServer(std::string listenerIP, int listenerPort, uint32_t nMaxConnections = 100);
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
    void sendRaw();
    //отправить данные всем клиентам
    void sendData(const DataBuffer& data);
    void sendData(const DataBufferStr& data);
    //получить статус
    status getStatus();
    //получить порт
    uint32_t getPort();
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
    SOCKET createListener();
    //закрывает интерфейсы winsock
    void cleanupWinsock();
    //цикл приема данных
    void handlingLoop();
    //закрыть соедиение под указанным номером
    void closeConnection(const uint32_t connectionNum);
    //добавить соединение в массив соединенией
    //возвращает номер соединения в массиве fds/clients
    uint32_t addConnection(const SOCKET newConneection);
    //закрывает и уничтожает массивы соединений fds/clients
    void cleanup();
    //отправляет len байт из массива data на сокет fd
    // ! len должен быть меньше либо равен размеру массива data
    static int sendAll(const Socket fd, const char *data, int& len);
    //получить len байт из массива data на сокет sock
    // ! len должен быть меньше либо равен размеру массива data
    //возвращает количество отосланных байт, 0 при отсоединении клиента, -1 или ошибке
    int recvAll(const Socket sock, char* data, const int len);

    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(const Client sock, DataBuffer& data);
    //получить пакет данных, возвращает 0 при отсоединении соединения, -1 при ошибке, иначе количество полученных байт
    int recvPacket(const Client sock, DataBufferStr& data);

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
    SOCKET listener;
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

    status getStatus();
    SOCKET getSocket();
    //пользовательская переменная
    std::any ref;
    // client();

    ~Client();
    UServer::Client::status sendPacket(const DataBuffer& data);
    UServer::Client::status sendPacket(const DataBufferStr& data);

    void disconnect();
private:
    //дескриптор сокета
    SOCKET fd;
    //статус
    Client::status _status = Client::status::disconnected;
};
