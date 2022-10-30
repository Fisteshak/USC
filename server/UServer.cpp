#include "UServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <iostream>
#include <string>
#include <atomic>
#include <thread>

UServer::UServer(int listenerPort, std::string listenerIP, uint32_t nMaxConnections) : listenerPort(listenerPort), listenerIP(listenerIP)
{
    if (nMaxConnections == 0) {
        throw std::runtime_error("nMaxConnections value should not be zero");
    }
    if (nMaxConnections > SOMAXCONN) {
        throw std::runtime_error("nMaxConnections value is too big");
    }
    this->nMaxConnections = nMaxConnections+1;

    fds.resize(nMaxConnections);
    clients.resize(nMaxConnections);

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
        for (int i = 1; i < nConnections; i++) {
            closesocket(fds[i].fd);
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
    for (int i = 1; i < nConnections; i++) {
        closesocket(fds[i].fd);
    }

    fds.clear();
    clients.clear();
}

uint32_t UServer::get_block_size()
{
	return block_size;
}

void UServer::set_block_size(uint32_t size)
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
		events_num = WSAPoll(fds.data(), nConnections, -1);   //ждать входящих событий

        //в случае если сервер был остановлен извне (.stop)
        if (_status != status::up) {
            return;
        }

        if (events_num == SOCKET_ERROR) {
            _status = status::error_wsapoll;
            cleanup();
            return;
        }

        int handled_events = 0;

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

                    nConnections++;
                    fds[nConnections - 1] = {new_conn, POLLIN, 0}; //добавить в массив fds

                    clients[nConnections - 1].fd = new_conn;
                    clients[nConnections - 1]._status = client::connected;

                    if (conn_handler) {
                        conn_handler(clients[nConnections - 1]);
                    }
                }
                handled_events++;
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
        for (int i = 1; handled_events < events_num; i++) {

            //if (fds[i].revents | POLLIN)       //если есть входящие данные
            if (fds[i].revents != 0)       //если есть входящие данные

            {
                int recieved_data;		//количество прочитанных байт
				recieved_data = recv(fds[i].fd, data_buf.data(), block_size, 0);  //прочитать

                //если мы успешно прочитали данные
                if (recieved_data > 0) {
                    if (data_handler) {
                        data_handler(data_buf, clients[i]);  //вызвать обработчик
                    }
                }

                //при нормальном закрытии соединения
                if (recieved_data == 0) {
                    if (disconn_handler) {
                        disconn_handler(clients[i]);
                    }
                    closesocket(fds[i].fd);
                    fds.erase(fds.begin()+i);      //удалить из массива соединений
                    clients.erase(clients.begin()+i);      //удалить из массива соединений

                    fds.push_back({});
                    clients.push_back({});

                    nConnections--;

                }

                //если recv вернул ошибку
                if (recieved_data == SOCKET_ERROR) {
                    //при "жестком" закрытии соединения
                    if (WSAGetLastError() == WSAECONNRESET) {
                        if (disconn_handler) {
                            disconn_handler(clients[i]);
                        }
                    }
                    closesocket(fds[i].fd);
                    fds.erase(fds.begin()+i);      //удалить из массива соединений
                    clients.erase(clients.begin()+i);      //удалить из массива соединений

                    fds.push_back({});
                    clients.push_back({});

                    nConnections--;

                }

                handled_events++;

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


void UServer::sendData(DataBuffer& data)
{

    for (int i = 1; i < nConnections; i++) {
        int data_len = send(fds[i].fd, data.data(), data.size(), 0);
        if (data_len < 0) { //если есть ошибка, то закрываем соединение
            std::cout << "Error sending data " << WSAGetLastError() << std::endl;
            // close_conn = true;					//закрываем соединение
        }
    }
    return;
}

void UServer::sendData(DataBufferStr& data)
{

    for (int i = 1; i < nConnections; i++) {
        int data_len = send(fds[i].fd, data.data(), data.size(), 0);
        if (data_len < 0) { //если есть ошибка, то закрываем соединение
            std::cout << "Error sending data " << WSAGetLastError() << std::endl;
            // close_conn = true;					//закрываем соединение
        }
    }
    return;
}

void UServer::set_data_handler(data_handler_t handler)
{
    data_handler = handler;
    return;
}

void UServer::set_conn_handler(conn_handler_t handler)
{
    conn_handler = handler;
    return;

}

void UServer::set_disconn_handler(conn_handler_t handler)
{
    disconn_handler = handler;
    return;
}

UServer::client::status UServer::client::getStatus()
{
    return _status;
}
SOCKET UServer::client::getSocket()
{
    return fd;
}

UServer::client::status UServer::client::sendData(DataBuffer& data)
{
    int dataLen = send(fd, data.data(), data.size(), 0);
    if (dataLen == SOCKET_ERROR) {
    _status = status::error_send_data;
    }

    return _status;
}

UServer::client::status UServer::client::sendData(DataBufferStr& data)
{
    int dataLen = send(fd, data.data(), data.size(), 0);
    if (dataLen == SOCKET_ERROR) {
        _status = status::error_send_data;
    }

    return _status;
}

UServer::client::~client()
{
    closesocket(this->fd);
}
