#include "UClient.h"
#include <iostream>
#include <string>


void data_handler(UClient::DataBuffer& data)
{
    // for (int i = 0; data[i] != '\0'; i++) {
    //     std::cout << data[i];
    // }
    for (int i = 0; i < data.size(); i++) {
        std::cout << data[i];
    }
    std::cout << std::endl;
}

void conn_handler()
{
    std::cout << "[client] Succesfully connected to server" << std::endl;
}

void disconn_handler()
{
    std::cout << "[client] Disconnected from server" << std::endl;
}

int main()
{
    UClient client;

    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);

    std::cout << "Write your name: " << std::endl;

    std::string name;
    std::getline(std::cin, name, '\n');

    client.connectTo("127.0.0.1", 9554);

    client.sendPacket(name);

    if (client.getStatus() != UClient::status::connected) {
        std::cout << "Failed to connect to a server\n";
        return 0;
    }

    UClient::DataBufferStr data;

    while (true) {


        std::getline(std::cin, data, '\n');

        if (data == ":stop" || client.getStatus() != UClient::status::connected) {
            break;
        }
        client.sendPacket(data);
    }


    client.disconnect();
}