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
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == INVALID_SOCKET) {
        return _status = status::error_socket_create;
    }

    //преобразовать IP в структуру in_addr
    in_addr clientIP;

	if (inet_pton(AF_INET, IP.data(), &clientIP) <= 0) {
        closesocket(clientSocket);
		return _status = status::error_socket_create;
	}

	//привязка к сокету адреса и порта
	sockaddr_in clientInfo;

	ZeroMemory(&clientInfo, sizeof(clientInfo));    //обнулить

	clientInfo.sin_family = AF_INET;
	clientInfo.sin_port = htons(port);
	clientInfo.sin_addr = clientIP;

    //подключиться к серверу
    auto connRes = connect(clientSocket, (sockaddr*)&clientInfo, sizeof(clientInfo));

    if (connRes != SOCKET_ERROR) {
        _status = status::connected;

        recvHandlingLoopThread = std::thread(&recvHandlingLoop, this);

        if (conn_handler) {
            conn_handler();
        }

    }
    else {
        closesocket(clientSocket);
        _status = status::error_socket_connect;
    }

    return _status;
}


void UClient::disconnect()
{
    _status = status::disconnected;
    closesocket(clientSocket);
    joinThreads();
}

void UClient::pause()
{
    _status == status::paused;
    return;
}

void UClient::recvHandlingLoop()
{
    while (_status == status::connected) {
        DataBuffer dataBuf(block_size);
        int messageSize = recv(clientSocket, dataBuf.data(), dataBuf.size(), 0);

        if (messageSize > 0) {
            if (data_handler) {
                data_handler(dataBuf);
            }
        }

    }

    return;
}

void UClient::joinThreads()
{
    if (recvHandlingLoopThread.joinable()) {
        recvHandlingLoopThread.join();
    }
    return;
}

UClient::status UClient::getStatus()
{
    return _status;
}

void UClient::set_data_handler(data_handler_t handler)
{
    data_handler = handler;
    return;
}

void UClient::set_conn_handler(conn_handler_t handler)
{
    conn_handler = handler;
    return;
}
