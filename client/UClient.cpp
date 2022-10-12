#include "UClient.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <iostream>

UClient::UClient()
{
    initWinsock();

}

UClient::~UClient()
{

}

bool UClient::initWinsock()
{
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &wsaData) != 0)
    {
        std::cout << "Error: can't initialize winsock!" << WSAGetLastError();        
        return false;
    }
    return true;
}

UClient::status UClient::connectTo(std::string IP, uint32_t port)
{



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

}