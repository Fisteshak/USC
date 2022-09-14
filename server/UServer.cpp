#include "UServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"
#include <iostream>
#include <string>

UServer::UServer(int listenerPort, std::string listenerIP) : listenerPort(listenerPort), listenerIP(listenerIP)
{
    
}

UServer::~UServer()
{
}

bool UServer::initWinsock()  //Инициализирует сетевой интерфейс для сокетов.
                         //Возвращает true в случае успеха, false в случае неудачи.
{
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2); //версия Winsock
    if (WSAStartup(ver, &wsaData) != 0)
    {
        std::cout << "Error: can't initialize winsock!" << WSAGetLastError();        
        return false;
    }
    return true;
}

SOCKET UServer::createListener() // создает сокет-слушатель    
{
    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) return INVALID_SOCKET;
    in_addr serv_ip;

    
	if (inet_pton(AF_INET, listenerIP.data(), &serv_ip) <= 0) {
        closesocket(listener);
		return INVALID_SOCKET;
	}

	// int on = 1;

	// if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
	// {
	// 	perror("setsockopt() failed");
	// 	closesocket(listenSock);
	// 	cin.get();
	// 	exit(1);
	// }

	// //перевести в неблок режим
	// DWORD nonBlocking = 1;
	// if (ioctlsocket(listenSock, FIONBIO, &nonBlocking) != 0)
	// {
	// 	cout << "Ошибка при переводе сокета в неблокирующий режим " << WSAGetLastError() << endl;
	// 	cin.get();
	// 	exit(1);
	// }

	// //привязка к сокету адреса и порта
	// sockaddr_in servInfo;

	// ZeroMemory(&servInfo, sizeof(servInfo));    //обнулить

	// servInfo.sin_family = AF_INET;
	// servInfo.sin_port = htons(PORT);
	// servInfo.sin_addr = serv_ip;

	// int servInfoLen = sizeof(servInfo);

	// if (bind(listenSock, (sockaddr*)&servInfo, servInfoLen) != 0) {
	// 	cout << "Ошибка привязки сокета к порту и IP-адресу " << WSAGetLastError() << endl;
	// 	closesocket(listenSock);
	// 	WSACleanup();
	// 	cin.get();
	// 	exit(1);
	// }
	// else if (debug)
	// 	cout << "Сокет успешно привязан" << endl;


}



