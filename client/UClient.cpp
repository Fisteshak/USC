#include "UClient.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include <iostream>

UClient::UClient()
{

}

UClient::~UClient()
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