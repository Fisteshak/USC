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

    if (connect(clientSocket, (sockaddr*)&clientInfo, sizeof(clientInfo)) != SOCKET_ERROR) {
        _status = status::connected;
        if (connHandler) {
            connHandler();
        }
        recvHandlingLoopThread = std::thread(&recvHandlingLoop, this);

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
        //получить данные
        int messageSize = recv(clientSocket, dataBuf.data(), dataBuf.size(), 0);

        if (messageSize > 0) {
            //вызвать обработчик
            if (dataHandler) {
                dataHandler(dataBuf);
            }
        }
        //при закрытии соединения со стороны сервера
        if (messageSize <= 0) {
            _status = status::disconnected;

            if (disconnHandler) {
                disconnHandler();
            }

            closesocket(clientSocket);
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

void UClient::setDataHandler(DataHandler handler)
{
    dataHandler = handler;
    return;
}

void UClient::setConnHandler(ConnHandler handler)
{
    connHandler = handler;
    return;

}

void UClient::setDisconnHandler(ConnHandler handler)
{
    disconnHandler = handler;
    return;
}

