#include "UServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <iostream>
#include <string>

#include "event2/event.h"
#include "event2/listener.h"



UServer::UServer()
{


}

UServer::~UServer()
{
}

bool initWinsock()
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