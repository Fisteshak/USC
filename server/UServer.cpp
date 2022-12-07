#include "UServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <any>

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
SOCKET UServer::createListener()
{

    //создать сокет
    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
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

    //преобразовать IP в структуру in_addr
    in_addr serv_ip;

	if (inet_pton(AF_INET, listenerIP.data(), &serv_ip) <= 0) {
        closesocket(listener);
		return INVALID_SOCKET;
	}

	//привязка к сокету адреса и порта
	sockaddr_in servInfo;

	ZeroMemory(&servInfo, sizeof(servInfo));    //обнулить

	servInfo.sin_family = AF_INET;
	servInfo.sin_port = htons(listenerPort);
	servInfo.sin_addr = serv_ip;

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

        int handledConnections = 0;
        for (int i = 1; handledConnections < nConnections; i++) {
            if (fds[i].fd != 0) {
                closesocket(fds[i].fd);
                handledConnections++;
            }
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

    int handledConnections = 0;
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

                    SOCKET new_conn = accept(listener, NULL, NULL); //принять соединение

                    if (new_conn == INVALID_SOCKET) { //если accept возвратил INVALID_SOCKET, то все соединения приняты
                        int err = WSAGetLastError();
                        if (WSAGetLastError() != WSAEWOULDBLOCK) {
                            _status = status::error_accept_connection;
                        }
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
                int recievedDataLen;		//количество прочитанных байт
				recievedDataLen = recv(fds[i].fd, data_buf.data(), block_size, 0);  //прочитать

                //если мы успешно прочитали данные
                if (recievedDataLen > 0) {
                    if (dataHandler) {
                        dataHandler(data_buf, clients[i]);  //вызвать обработчик
                    }
                }


                //при нормальном закрытии соединения
                if (recievedDataLen == 0) {
                    if (disconnHandler) {
                        disconnHandler(clients[i]);
                    }
                    closeConnection(i);
                }

                //TODO: добавить обработчик ошибок
                //если recv вернул ошибку
                //сокет закрываем в любом случае
                if (recievedDataLen == SOCKET_ERROR) {
                    //при "жестком" закрытии соединения

                    if (WSAGetLastError() == WSAECONNRESET) {
                        if (disconnHandler) {
                            disconnHandler(clients[i]);
                        }
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

    SOCKET listener = createListener();  //создать сокет-слушатель

    fds[0] = {listener, POLLIN, 0};  //поместить слушателя в начало массива дескрипторов сокетов

    clients[0].fd = listener;

    nConnections = 1;

    this->listener = listener;

	if (listener == INVALID_SOCKET) {
		std::cout << "create listener error" << std::endl;
		return _status = error_listener_create;
	}
	//запустить поток обработки входящих сообщений
	handlingLoopThread = std::thread(&handlingLoop, this);
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
    closesocket(fds[connectionNum].fd);

    //сбросить параметры
    fds[connectionNum].fd = 0;
    fds[connectionNum].events = 0;
    clients[connectionNum].ref.reset();
    clients[connectionNum].fd = 0;

    nConnections--;
}

/*
добавляет соединение в первую пустую ячейку
*/

uint32_t UServer::addConnection(const SOCKET newConnection)
{
    //ищем первую пустую ячейку
    auto firstEmpty = find_if(fds.begin(), fds.end(), [](const pollfd& sock) { return sock.fd == 0; });
    int firstEmptyInd = firstEmpty - fds.begin();

    nConnections++;

    fds[firstEmptyInd] = { newConnection, POLLIN, 0 }; //добавить в массив fds

    clients[firstEmptyInd].fd = fds[firstEmptyInd].fd;
    clients[firstEmptyInd]._status = Client::connected;

    return firstEmptyInd;
}


int UServer::sendAll(const Socket fd, const char *data, int& len)
{
    int total = 0;       // сколько байт отправили
    int bytesleft = len; // сколько байт осталось отправить
    int n;

    //упаковываем размер в массив на 4 байта
    char dataLen[4]{};
    *(int *)dataLen = len;
    //отправляем размер
    n = send(fd, dataLen, 4, 0);

    while(total < len) {
        n = send(fd, data+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // возвратить количество отправленных байт

    return n==-1?-1:0; // вернуть -1 при ошибке, 0 при нормальном завершении
}

void UServer::sendData(const DataBuffer& data)
{
    int handledConnections = 1;

    for (int i = 1; handledConnections < nConnections; i++) {
        if (fds[i].fd != 0) {

            int len = data.size();
            int err = sendAll(fds[i].fd, data.data(), len);

            if (err < 0) {
                std::cout << "Error sending data to Socket №" << fds[i].fd << std::endl;
                std::cout << "Error №" << WSAGetLastError() << std::endl;
            }

            handledConnections++;
        }
    }
    return;
}

void UServer::sendData(const DataBufferStr& data)
{
    int handledConnections = 1;

    for (int i = 1; handledConnections < nConnections; i++) {
        if (fds[i].fd != 0) {

            int len = data.size();
            int err = sendAll(fds[i].fd, data.data(), len);

            if (err < 0) {
                std::cout << "Error sending data to Socket №" << fds[i].fd << std::endl;
                std::cout << "Error №" << WSAGetLastError() << std::endl;
            }

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

UServer::Client::status UServer::Client::getStatus()
{
    return _status;
}

SOCKET UServer::Client::getSocket()
{
    return fd;
}

UServer::Client::status UServer::Client::sendData(const DataBuffer& data)
{
    int len = data.size();
    int dataLen = sendAll(fd, data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        _status = status::error_send_data;
    }

    return _status;
}

UServer::Client::status UServer::Client::sendData(const DataBufferStr& data)
{
    int len = data.size();
    int dataLen = sendAll(fd, data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        _status = status::error_send_data;
    }

    return _status;
}

UServer::Client::~Client()
{
    closesocket(this->fd);
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