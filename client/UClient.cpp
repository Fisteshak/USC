#include <iostream>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include "windows.h"

#include "UClient.h"

UClient::UClient()
{
    initWinsock();
}

UClient::~UClient()
{
    disconnect();
}

bool UClient::initWinsock()
{
    WSADATA wsaData;
    WORD ver = MAKEWORD(2, 2);
    if (WSAStartup(ver, &wsaData) != 0)
    {
        std::cout << "Error: can't initialize winsock!" << getLastError();
        return false;
    }
    return true;
}

UClient::status UClient::connectTo(const std::string& IP, const uint32_t port)
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
        if (connHandler) {
            connHandler();
        }
        recvHandlingLoopThread = std::thread(&UClient::recvHandlingLoop, this);

    }
    else {
        closesocket(clientSocket);
        _status = status::error_socket_connect;
    }

    return _status;
}


int UClient::sendAll(const char *data, const int len)
{
    int total = 0;       // сколько байт отправили
    int bytesleft = len; // сколько байт осталось отправить
    int n;

    //упаковываем размер в массив на 4 байта
    char dataLen[4]{};
    *(int*)dataLen = len;

    //отправляем размер
    n = send(clientSocket, dataLen, 4, 0);

    if (n == SOCKET_ERROR) {
        return n;
    }

    while (total < len) {
        n = send(clientSocket, data+total, bytesleft, 0);
        if (n == SOCKET_ERROR) {
            return n;
        }
        total += n;
        bytesleft -= n;
    }


    return total;

}


void UClient::disconnect()
{
    if (_status != status::disconnected) {
        _status = status::disconnected;
        closesocket(clientSocket);
        joinThreads();
    }
}

// void UClient::pause()
// {
//     _status == status::paused;
//     return;
// }

void UClient::recvHandlingLoop()
{
    DataBuffer dataBuf;
    while (_status == status::connected) {
        //получить данные

        int messageSize = recvPacket(dataBuf);
        //std::cout << "Message size is " << messageSize << std::endl;
        if (_status != status::connected) {
            break;
        }

        if (messageSize > 0) {
            //вызвать обработчик
            if (dataHandler) {
                dataHandler(dataBuf);
            }
        }
        //при закрытии соединения со стороны сервера
        if (messageSize <= 0) {
            if (messageSize == 0) _status = status::disconnected;
            else _status = status::error_recv_data;

            //вызвать обработчик
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

UClient::status UClient::sendPacket(const DataBuffer& data)
{
    if (_status != status::connected or data.size() == 0) {
        return _status;
    }

    int len = data.size();
    int dataLen = sendAll(data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        disconnect();
        _status = status::error_send_data;
    }

    return _status;
}

UClient::status UClient::sendPacket(const DataBufferStr& data)
{
    if (_status != status::connected or data.size() == 0) {
        return _status;
    }

    int len = data.size();
    int dataLen = sendAll(data.data(), len);

    if (dataLen == SOCKET_ERROR) {
        disconnect();
        _status = status::error_send_data;
    }

    return _status;
}

int UClient::recvAll(char* data, const int len) {
    int total = 0;
    int received;

    while (total < len) {
        received = recv(clientSocket, data + total, len - total, 0);
        if (received == SOCKET_ERROR) {
            //std::cout << "Error recieving data " << getLastError() << std::endl;
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

int UClient::recvPacket(DataBuffer& data)
{
    char dataLenArr[4]{};

    //получить 4 байта длины
    int recievedData = recvAll(dataLenArr, 4);

    if (recievedData <= 0)  {
        return recievedData;
    }

    int dataLen = *(int *)dataLenArr;
    data.resize(dataLen);

    recievedData = recvAll(data.data(), dataLen);

    return recievedData;
}

int UClient::recvPacket(DataBufferStr& data)
{
    char dataLenArr[4]{};

    //получить 4 байта длины
    int recievedData = recvAll(dataLenArr, 4);

    if (recievedData <= 0)  {
        return recievedData;
    }

    int dataLen = *(int *)dataLenArr;
    data.resize(dataLen);

    recievedData = recvAll(data.data(), dataLen);

    return recievedData;
}

UClient::status UClient::getStatus()
{
    return _status;
}

void UClient::setDataHandler(const DataHandler handler)
{
    dataHandler = handler;
    return;
}

void UClient::setConnHandler(const ConnHandler handler)
{
    connHandler = handler;
    return;

}

void UClient::setDisconnHandler(const ConnHandler handler)
{
    disconnHandler = handler;
    return;
}


int UClient::getLastError()
{
    return WSAGetLastError();
}

/*
⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⠛⠛⠋⠉⠈⠉⠉⠉⠉⠛⠻⢿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⡿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⢿⣿⣿⣿⣿
⣿⣿⣿⣿⡏⣀⠀⠀⠀⠀⠀⠀⠀⣀⣤⣤⣤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿
⣿⣿⣿⢏⣴⣿⣷⠀⠀⠀⠀⠀⢾⣿⣿⣿⣿⣿⣿⡆⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿
⣿⣿⣟⣾⣿⡟⠁⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣷⢢⠀⠀⠀⠀⠀⠀⠀⢸⣿
⣿⣿⣿⣿⣟⠀⡴⠄⠀⠀⠀⠀⠀⠀⠙⠻⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⣿
⣿⣿⣿⠟⠻⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠶⢴⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⣿
⣿⣁⡀⠀⠀⢰⢠⣦⠀⠀⠀⠀⠀⠀⠀⠀⢀⣼⣿⣿⣿⣿⣿⡄⠀⣴⣶⣿⡄⣿
⣿⡋⠀⠀⠀⠎⢸⣿⡆⠀⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⠗⢘⣿⣟⠛⠿⣼
⣿⣿⠋⢀⡌⢰⣿⡿⢿⡀⠀⠀⠀⠀⠀⠙⠿⣿⣿⣿⣿⣿⡇⠀⢸⣿⣿⣧⢀⣼
⣿⣿⣷⢻⠄⠘⠛⠋⠛⠃⠀⠀⠀⠀⠀⢿⣧⠈⠉⠙⠛⠋⠀⠀⠀⣿⣿⣿⣿⣿
⣿⣿⣧⠀⠈⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠟⠀⠀⠀⠀⢀⢃⠀⠀⢸⣿⣿⣿⣿
⣿⣿⡿⠀⠴⢗⣠⣤⣴⡶⠶⠖⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡸⠀⣿⣿⣿⣿
⣿⣿⣿⡀⢠⣾⣿⠏⠀⠠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠛⠉⠀⣿⣿⣿⣿
⣿⣿⣿⣧⠈⢹⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⣿
⣿⣿⣿⣿⡄⠈⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣴⣾⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣦⣄⣀⣀⣀⣀⠀⠀⠀⠀⠘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡄⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠙⣿⣿⡟⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⠀⠁⠀⠀⠹⣿⠃⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⡿⠛⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⢐⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⠿⠛⠉⠉⠁⠀⢻⣿⡇⠀⠀⠀⠀⠀⠀⢀⠈⣿⣿⡿⠉⠛⠛⠛⠉⠉
⣿⡿⠋⠁⠀⠀⢀⣀⣠⡴⣸⣿⣇⡄⠀⠀⠀⠀⢀⡿⠄⠙⠛⠀⣀⣠⣤⣤⠄⠀
*/