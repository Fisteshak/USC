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
        //получить данные

        //получить 4 байта длины
        char dataLenArr[4]{};
        recv(clientSocket, dataLenArr, 4, 0);

        int dataLen = *(int *)dataLenArr;
        std::cout << "Message size is " << dataLen << " bytes." << std::endl;

        DataBuffer dataBuf(dataLen);

        int messageSize = recv(clientSocket, dataBuf.data(), dataLen, 0);

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
            _status = status::disconnected;
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

UClient::status UClient::sendDataToServer(const DataBuffer& data)
{
    if (_status == status::connected) {
        int dataLen = send(clientSocket, data.data(), data.size(), 0);
        if (dataLen < 0) {
            std::cout << "Error sending data " << getLastError() << std::endl;
            _status = status::error_send_data;
        }
    }
    return _status;
}

UClient::status UClient::sendDataToServer(const DataBufferStr& data)
{
    if (_status == status::connected) {
        int dataLen = send(clientSocket, data.data(), data.size(), 0);
        if (dataLen < 0) {
            std::cout << "Error sending data " << getLastError() << std::endl;
            _status = status::error_send_data;
        }
    }
    return _status;
}

int UClient::recvAll(const Socket fd, const char *data, int& len)
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

int UClient::recvAll(Socket clientSocket, char *data)
{
    int total = 0;       // сколько байт отправили
    int bytesleft;       // сколько байт осталось отправить
    int n;


    char dataLenArr[4]{};
    recv(clientSocket, dataLenArr, 4, 0);

    int dataLen = *(int *)dataLenArr;
    //std::cout << "Message size is " << dataLen << " bytes." << std::endl;

    int messageSize = recv(clientSocket, data,  dataLen, 0);
}

UClient::status UClient::getStatus()
{
    return _status;
}

uint32_t UClient::getBlockSize()
{
	return blockSize;
}

void UClient::setBlockSize(const uint32_t size)
{
	blockSize = size;
    return;
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
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢀⣀⣤⣴⣶⣾⣆⠄⠄⠄⠄⠄
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠠⣤⣤⣤⠄⢤⣴⣶⣿⣿⣿⢟⣿⣿⣿⠄⠄⠄⠄⠄
⠄⠄⠄⠄⠄⣀⣠⣤⡾⢏⢠⣶⣶⣾⡑⡀⢸⠋⠁⠄⢠⣾⣿⣿⣿⠄⠄⠄⠄⠄
⠄⢀⣠⣶⣿⣿⣿⠟⠁⠁⠰⣋⢫⢲⡇⠛⠄⠄⢀⣠⣤⣉⠻⠿⠿⠄⠄⠄⠄⠄
⢰⣿⣿⣟⣋⣉⣁⠄⠄⠄⠄⠻⣆⣂⡕⠼⠂⠉⣿⣿⡇⢏⠉⠉⠁⠄⠄⠄⠄⠄
⠄⠙⢿⣿⣿⣿⣿⠃⢸⢋⡁⢊⠒⣲⡶⠊⢁⣴⣿⣿⣿⣦⣦⠄⠄⠄⠄⠄⠄⠄
⠄⠄⠄⠙⠻⣿⡟⠄⡆⢸⣧⣾⣶⣤⣤⣾⡿⣿⣿⠿⡻⣻⣿⠁⠄⠄⠄⠄⠄⠄
⠄⠄⠄⠄⠄⠄⠄⠘⠠⣾⣿⣿⡿⠿⠿⣥⣾⣿⠿⣛⠅⢰⣗⠄⠄⠄⠄⠄⠄⠄
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠸⣶⣶⣿⣟⣛⣛⣛⠲⠿⣵⣿⣟⢅⠂⠄⠄⠄⠄⠄⠄
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢹⡟⠋⣠⣤⣤⣤⣍⡑⠂⠬⢉⣾⠄⠄⠄⠄⠄⠄⠄
 ⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢠⠁⣿⣿⣿⣿⠿⠿⠿⠿⠷⢶⡄⠄⠄⠄⠄⠄⠄⠄
 ⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠘⠄⢉⣁⣀⣤⣤⣤⣤⣤⣤⣤⠄⠄⠄⠄⠄⠄⠄⠄
 ⠄⠄⠄⠄⠄⠄⠄⠄⠄⣀⠈⠙⠿⣿⣿⣿⣿⡍⠟⢁⣠⣤⣶⣶⣤⣄⡀⠄⠄⠄
 ⠄⠄⠄⠄⠄⠄⠄⢀⣾⣿⣿⣷⣦⣀⠉⠿⣿⡇⠄⣾⣿⣿⣿⣿⣿⣿⣿⣶⡀⠄
 ⠄⠄⠄⠄⠄⠄⢠⣾⣿⣿⣿⣿⣿⣿⣷⡄⠹⠇⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠄
 ⠄⠄⠄⠄⠄⠄⣾⣿⣿⣿⣿⣿⣿⣿⡿⠋⠄⠄⠈⠙⠿⢿⣿⣿⣿⣿⣿⣿⣿⡇
*/