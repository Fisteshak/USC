#include "UServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <iostream>
#include <string>
#include <atomic>
#include <thread>

UServer::UServer(int listenerPort, std::string listenerIP, uint32_t max_connections) : listenerPort(listenerPort), listenerIP(listenerIP)
{
    if (max_connections == 0) {
        throw std::runtime_error("max_connections value should not be zero");
    }
    if (max_connections > SOMAXCONN) {
        throw std::runtime_error("max_connections value is too big");
    }
    this->max_connections = max_connections;
}

UServer::~UServer()
{
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
	_status = status::stopped;
}       


UServer::status UServer::handlingLoop()
{
	while (_status == status::up) {
	
	
	
	}

}


//запускает сервер (цикл приема данных)
UServer::status UServer::run()
{
	_status = status::up;	

	//инициализоровать Winsock
	if (!initWinsock()) {
		return status::error_winsock_init;
	}

    SOCKET listener = createListener();  //создать сокет-слушатель

	if (listener == INVALID_SOCKET) {
		std::cout << "create listener error" << std::endl;
		return error_listener_create;
	}

	std::thread hadlingLoopThread(handlingLoop);
	
}

