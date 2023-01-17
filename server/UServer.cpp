#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <any>
#include <algorithm>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <ciso646>

#include "UServer.h"
#include "../crypto/aes.h"

UServer::UServer(std::string listenerIP, int listenerPort, uint32_t nMaxConnections) : listenerPort(listenerPort), listenerIP(listenerIP)
{
    if (nMaxConnections == 0) {
        throw std::runtime_error("nMaxConnections value should not be zero");
    }
    if (nMaxConnections > SOMAXCONN) {
        throw std::runtime_error("nMaxConnections value is too big");
    }
    this->nMaxConnections = nMaxConnections+1;

    fds.resize(this->nMaxConnections);
    clients.resize(this->nMaxConnections);

    return;
}

UServer::UServer(int listenerPort, uint32_t nMaxConnections) : listenerPort(listenerPort)
{
    if (nMaxConnections == 0) {
        throw std::runtime_error("nMaxConnections value should not be zero");
    }
    if (nMaxConnections > SOMAXCONN) {
        throw std::runtime_error("nMaxConnections value is too big");
    }
    this->nMaxConnections = nMaxConnections+1;

    fds.resize(this->nMaxConnections);
    clients.resize(this->nMaxConnections);
    listenerIP.clear();

    return;
}

UServer::~UServer()
{
    if (_status == status::up) {
        stop();
    }

    cleanupWinsock();
    return;
}

//Инициализирует сетевой интерфейс для сокетов.
//Возвращает true в случае успеха, false в случае неудачи.
bool UServer::initWinsock()
{
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2); //версия Winsock
    if (WSAStartup(ver, &wsaData) != 0)
    {
        return false;
    }
    return true;
}

//закрывает интерфейсы winsock
void UServer::cleanupWinsock()
{
    WSACleanup();
    return;

}

// создает сокет-слушатель
Socket UServer::createListener()
{


    //создать сокет
    Socket listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) return INVALID_SOCKET;

    //позволяет системе использовать только что закрытое соединение
    //(например в случае быстрого перезапуска программы)
	int on = 1;
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	{
		closesocket(listener);
		return INVALID_SOCKET;
	}

    //перевести сокет в неблокирующий режим
    DWORD nonBlocking = 1;
    if (ioctlsocket(listener, FIONBIO, &nonBlocking) < 0)
    {
        closesocket(listener);
		return INVALID_SOCKET;
    }


    if (!listenerIP.empty()) {
        //если IP для сервера указан
    }

	//привязка к сокету адреса и порта

	sockaddr_in servInfo;
	ZeroMemory(&servInfo, sizeof(servInfo));    //обнулить

	servInfo.sin_family = AF_INET;
	servInfo.sin_port = htons(listenerPort);

    //если IP указан
    if (!listenerIP.empty()) {
        //преобразовать IP в структуру in_addr
        int err;
        in_addr serv_ip;
        err = inet_pton(AF_INET, listenerIP.data(), &serv_ip);
	    if (err <= 0) {
            closesocket(listener);
		    return INVALID_SOCKET;
	    }
        //установить
    	servInfo.sin_addr = serv_ip;
    }
    else {
        //иначе использовать любой
        servInfo.sin_addr.s_addr = INADDR_ANY;
    }

	int servInfoLen = sizeof(servInfo);

	if (bind(listener, (sockaddr*)&servInfo, servInfoLen) != 0) {
		closesocket(listener);
		cleanupWinsock();
		return INVALID_SOCKET;
	}


    if (listen(listener, SOMAXCONN) < 0)
    {
        closesocket(listener);
		cleanupWinsock();
		return INVALID_SOCKET;
    }

    return listener;
}
void UServer::stop()
{
    if (_status != status::stopped) {

        _status = status::stopped;

        closesocket(listener);

        uint32_t handledConnections = 0;
        for (int i = 1; handledConnections < nConnections; i++) {
            closeConnection(i);
        }

        joinThreads();

        fds.clear();
        clients.clear();
    }
    return;
}

void UServer::joinThreads()
{
	handlingLoopThread.join();
    return;
}

void UServer::cleanup()
{
    closesocket(listener);

    uint32_t handledConnections = 0;
    for (int i = 1; handledConnections < nConnections; i++) {
        if (fds[i].fd != 0) {
            closesocket(fds[i].fd);
            handledConnections++;
        }
    }

    fds.clear();
    clients.clear();
}

uint32_t UServer::getBlockSize()
{
	return block_size;
}

void UServer::setBlockSize(const uint32_t size)
{
	block_size = size;
    return;
}

UServer::status UServer::getStatus()
{
    return _status;
}

uint32_t UServer::getPort()
{
    return listenerPort;
}

bool UServer::setPort(const uint32_t& port)
{
    if (_status == status::up) return false;

    this->listenerPort = port;
    return true;
}

bool UServer::setIP(const std::string& IP)
{
    if (_status == status::up) return false;
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, IP.c_str(), &(sa.sin_addr));
    if (result != 0) {
        listenerIP = IP;
        return true;
    }
    else {
        return false;
    }
}


uint32_t UServer::Client::getIP()
{
    sockaddr_in client_info = {0};
    int addrsize = sizeof(client_info);

    getpeername(getSocket(), (sockaddr*)&client_info, &addrsize);

    return inet_addr(inet_ntoa(client_info.sin_addr));
}

std::string UServer::Client::getIPstr()
{
    sockaddr_in client_info = {0};
    int addrsize = sizeof(client_info);

    getpeername(getSocket(), (sockaddr*)&client_info, &addrsize);
    return std::string(inet_ntoa(client_info.sin_addr));
}


void UServer::handlingLoop()
{
	while (_status == status::up) {
        uint32_t events_num;    //количество полученных событий от сокетов
		events_num = WSAPoll(fds.data(), nMaxConnections, -1);   //ждать входящих событий

        //в случае если сервер был остановлен извне (.stop)
        if (_status != status::up) {
            return;
        }

        if (events_num == SOCKET_ERROR) {
            _status = status::error_wsapoll;
            cleanup();
            return;
        }

        int handledEvents = 0;

        //если на слушателе есть событие
        if (fds[0].revents != 0) {
            //то это входящее соединение
            if (fds[0].revents == POLLRDNORM) {
                while (true) {
                    if (nConnections == nMaxConnections) break;

                    Socket new_conn = accept(listener, NULL, NULL); //принять соединение

                    if (new_conn == INVALID_SOCKET) { //если accept возвратил INVALID_SOCKET, то все соединения приняты
                        int err = WSAGetLastError();
                        if (WSAGetLastError() != WSAEWOULDBLOCK) {
                            _status = status::error_accept_connection;
                        }
                        break;
                    }

                    //перевести сокет в блокирующий режим
                    DWORD Blocking = 0;
                    if (ioctlsocket(new_conn, FIONBIO, &Blocking) < 0) {
                        closesocket(new_conn);
                        break;
                    }

                    //добавить в массив соединений
                    int newConnInd = addConnection(new_conn);



                    if (connHandler) {
                        connHandler(clients[newConnInd]);
                    }

                }
                handledEvents++;
            }
            //иначе это ошибка
            else {
                _status = status::error_wsapoll;
            }
        }

        if (_status != status::up) {
            cleanup();
            return;
        }

		//перебрать все сокеты до тех пор пока не обработаем все полученные события
        DataBuffer data_buf(block_size);		//буфер для данных
        for (int i = 1; handledEvents < events_num; i++) {
            if (fds[i].revents != 0 && fds[i].fd != 0)       //если есть входящие данные и сокет не пустой
            {

                int packetSize;		//количество прочитанных байт
				packetSize = recvPacket(clients[i], data_buf);

                // получить 4 байта длины
                // char dataLenArr[4]{};
                // int recievedDataLen = recvAll(clients[i].fd, dataLenArr, 4);

                // int dataLen = *(int*)dataLenArr;
                // data_buf.resize(dataLen);
                // std::cout << "Message size is " << dataLen << std::endl;

                // recievedDataLen = recvAll(clients[i].fd, data_buf.data(), dataLen);

                //если мы успешно прочитали данные
                if (packetSize > 0) {
                    if (dataHandler) {
                        dataHandler(data_buf, clients[i]);  //вызвать обработчик
                    }
                }

                //при нормальном закрытии соединения
                if (packetSize == 0) {
                    if (disconnHandler) {
                        disconnHandler(clients[i]);
                    }
                    closeConnection(i);
                }

                //TODO: добавить обработчик ошибок
                //если recv вернул ошибку
                //сокет закрываем в любом случае
                if (packetSize == SOCKET_ERROR) {
                    //при "жестком" закрытии соединения

                    if (disconnHandler) {
                        disconnHandler(clients[i]);
                    }

                    closeConnection(i);
                }

                handledEvents++;

            }
        }
	}
	return;
}


//запускает сервер (цикл приема данных)
UServer::status UServer::run()
{
    if (_status == status::up) {
        stop();
    }

	_status = status::up;

	//инициализировать Winsock
	if (!initWinsock()) {
		return _status = status::error_winsock_init;
	}

    Socket listener = createListener();  //создать сокет-слушатель

    fds[0] = {listener, POLLIN, 0};  //поместить слушателя в начало массива дескрипторов сокетов

    clients[0].fd = listener;

    nConnections = 1;

    this->listener = listener;

	if (listener == INVALID_SOCKET) {
		std::cout << "create listener error" << std::endl;
		return _status = error_listener_create;
	}
	//запустить поток обработки входящих сообщений
	handlingLoopThread = std::thread(&UServer::handlingLoop, this);
    //handlingLoop();

	return _status;
}

/*
При закрытии соединения оставляем пустое место,
на которое позже запишется новое соединение.
Менять местами элементы нельзя, т.к. это поломает
ссылки на них со стороны пользователя.
*/

void UServer::closeConnection(const uint32_t connectionNum)
{
    if (fds[connectionNum].fd != 0) {
        closesocket(fds[connectionNum].fd);
        fds[connectionNum].fd = 0;
        clients[connectionNum].fd = 0;
    }

    //сбросить параметры

    fds[connectionNum].events = 0;
    clients[connectionNum].ref.reset();

    nConnections--;
    return;
}

/*
добавляет соединение в первую пустую ячейку
*/

uint32_t UServer::addConnection(const Socket newConnection)
{
    //ищем первую пустую ячейку
    auto firstEmpty = find_if(fds.begin(), fds.end(), [](const pollfd& sock) { return sock.fd == 0; });
    int firstEmptyInd = firstEmpty - fds.begin();

    nConnections++;

    fds[firstEmptyInd] = { newConnection, POLLIN, 0 }; //добавить в массив fds

    clients[firstEmptyInd].fd = fds[firstEmptyInd].fd;
    clients[firstEmptyInd]._status = Client::connected;
    clients[firstEmptyInd].owner = this;

    return firstEmptyInd;
}


int UServer::sendAll(const Socket fd, const char *data, const int len)
{
    int total = 0;       // сколько байт отправили
    int bytesleft = len; // сколько байт осталось отправить
    int n;

    //упаковываем размер в массив на 4 байта
    char dataLen[4]{};
    *(int *)dataLen = len;

    //отправляем размер
    n = send(fd, dataLen, 4, 0);

    if (n == SOCKET_ERROR) {
        return n;
    }

    while(total < len) {
        n = send(fd, data+total, bytesleft, 0);
        if (n == SOCKET_ERROR) {
            return n;
        }
        total += n;
        bytesleft -= n;
    }

    return total;
}

int UServer::recvAll(const Socket sock, char* data, const int len) {
    int total = 0;
    int received;

    while (total < len) {
        received = recv(sock, data + total, len - total, 0);
        if (received == SOCKET_ERROR) {
            //std::cout << "Error recieving data " << WSAGetLastError() << std::endl;
            return SOCKET_ERROR;
        }
        if (received == 0) {
            // disconnected
            return 0;
        }
        total += received;
    }

    return total;
}

int UServer::recvPacket(const UServer::Client& sock, DataBuffer& data)
{
    char dataLenArr[4]{};

    //получить 4 байта длины
    int recievedData = recvAll(sock.fd, dataLenArr, 4);

    if (recievedData <= 0)  {
        return recievedData;
    }

    int dataLen = *(int *)dataLenArr;
    data.resize(dataLen);

    recievedData = recvAll(sock.fd, (char*)data.data(), dataLen);

    return recievedData;
}

int UServer::recvPacket(const UServer::Client& sock, DataBufferStr& data)
{
    char dataLenArr[4]{};

    //получить 4 байта длины
    int recievedData = recvAll(sock.fd, dataLenArr, 4);

    if (recievedData <= 0)  {
        return recievedData;
    }

    int dataLen = *(int *)dataLenArr;
    data.resize(dataLen);

    recievedData = recvAll(sock.fd, data.data(), dataLen);

    return recievedData;
}

void UServer::sendPacket(const DataBuffer& data)
{
    int handledConnections = 1;

    for (int i = 1; handledConnections < nConnections; i++) {
        if (fds[i].fd != 0) {

            int err = clients[i].sendPacket(data);

            if (err < 0) {
                std::cout << "Error sending data to Socket №" << fds[i].fd << std::endl;
                std::cout << "Error №" << WSAGetLastError() << std::endl;
            }

            handledConnections++;
        }
    }
    return;
}

void UServer::sendPacket(const DataBufferStr& data)
{
    int handledConnections = 1;

    for (int i = 1; handledConnections < nConnections; i++) {
        if (fds[i].fd != 0) {

            clients[i].sendPacket(data);

            // if (err < 0) {
            //     std::cout << "Error sending data to Socket №" << fds[i].fd << std::endl;
            //     std::cout << "Error №" << WSAGetLastError() << std::endl;
            // }

            handledConnections++;
        }
    }
    return;
}

void UServer::setDataHandler(const DataHandler handler)
{
    dataHandler = handler;
    return;
}

void UServer::setConnHandler(const ConnHandler handler)
{
    connHandler = handler;
    return;
}

void UServer::setDisconnHandler(const ConnHandler handler)
{
    disconnHandler = handler;
    return;
}


void UServer::disconnect(UServer::Client& cl)
{
    auto clNum = find(clients.begin(), clients.end(), cl) - clients.begin();
    if (disconnHandler) {
        disconnHandler(clients[clNum]);
    }
    closeConnection(clNum);
    return;
}


UServer::Client::status UServer::Client::getStatus()
{
    return _status;
}

Socket UServer::Client::getSocket()
{
    return fd;
}

UServer::Client::status UServer::Client::sendPacket(const DataBuffer& data)
{
    if (_status != status::connected or data.size() == 0) {
        return _status;
    }

    int len = data.size();
    int dataLen = sendAll(fd, (char*)data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        _status = status::error_send_data;

        if (owner->disconnHandler) {
            owner->disconnHandler(*this);
        }

        int clientInd = std::find(owner->clients.begin(), owner->clients.end(), *this) - owner->clients.begin();
        owner->closeConnection(clientInd);
    }

    return _status;
}

UServer::Client::status UServer::Client::sendPacket(const DataBufferStr& data)
{
    if (_status != status::connected or data.size() == 0) {
        return _status;
    }

    int len = data.size();
    int dataLen = sendAll(fd, data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        _status = status::error_send_data;

        if (owner->disconnHandler) {
            owner->disconnHandler(*this);
        }

        int clientInd = std::find(owner->clients.begin(), owner->clients.end(), *this) - owner->clients.begin();
        owner->closeConnection(clientInd);
    }

    return _status;
}



UServer::Client::~Client()
{
    //closesocket(this->fd);
//     int clientInd = std::find(owner->clients.begin(), owner->clients.end(), *this) - owner->clients.begin();
//     owner->closeConnection(clientInd);
}

/*
⠄⠄⣿⣿⣿⣿⠘⡿⢛⣿⣿⣿⣿⣿⣧⢻⣿⣿⠃⠸⣿⣿⣿⠄⠄⠄⠄⠄
⠄⠄⣿⣿⣿⣿⢀⠼⣛⣛⣭⢭⣟⣛⣛⣛⠿⠿⢆⡠⢿⣿⣿⠄⠄⠄⠄⠄
⠄⠄⠸⣿⣿⢣⢶⣟⣿⣖⣿⣷⣻⣮⡿⣽⣿⣻⣖⣶⣤⣭⡉⠄⠄⠄⠄⠄
⠄⠄⠄⢹⠣⣛⣣⣭⣭⣭⣁⡛⠻⢽⣿⣿⣿⣿⢻⣿⣿⣿⣽⡧⡄⠄⠄⠄
⠄⠄⠄⠄⣼⣿⣿⣿⣿⣿⣿⣿⣿⣶⣌⡛⢿⣽⢘⣿⣷⣿⡻⠏⣛⣀⠄⠄
⠄⠄⠄⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⠙⡅⣿⠚⣡⣴⣿⣿⣿⡆⠄
⠄⠄⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠄⣱⣾⣿⣿⣿⣿⣿⣿⠄
⠄⢀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢸⣿⣿⣿⣿⣿⣿⣿⣿⠄
⠄⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠣⣿⣿⣿⣿⣿⣿⣿⣿⣿⠄
⠄⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠛⠑⣿⣮⣝⣛⠿⠿⣿⣿⣿⣿⠄
⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⠄⠄⠄⠄⣿⣿⣿⣿⣿⣿⣿⣿⣿⡟⠄
*/