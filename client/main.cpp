#include "UClient.h"
#include <iostream>
#include <string>

UClient client(UClient::CRYPTO_ENABLED);

void data_handler(UClient::DataBuffer& data)
{

    for (int i = 0; i < data.size(); i++) {
        std::cout << data[i];
    }
    std::cout << std::endl;
    //if (data.back() != '0') data.push_back('0');
    //std::cout << data.data() << std::endl;
}

void conn_handler()
{
    std::cout << "[client] Succesfully connected to server" << std::endl;
}

void disconn_handler()
{
    std::cout << "[client] Disconnected from server" << std::endl;
}

void parseIPAndPort(const string& IP_Port, string& IP, uint32_t& port)
{

    if (IP_Port.find(":") != string::npos) {
        //IP + port
        try {
            port = (stoul(IP_Port.substr(IP_Port.find(":") + 1, IP_Port.size())));
        } catch (std::exception) {
            std::cout << "Invalid port\n";
            exit(1);
        }
        IP = (IP_Port.substr(0, IP_Port.find(":")));

    } else {
        if (IP_Port.find('.') != string::npos) {
            //IP
            IP = (IP_Port.substr(0, IP_Port.size()));
        }
        else {
            //Port
            try {
                port = (stoul(IP_Port.substr(0, IP_Port.size())));
            } catch (std::exception) {
                std::cout << "Invalid port\n";
                exit(1);
            }
        }
    }
}

int main(int argc, char *argv[])
{

    //UClient client;

    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);

    std::string IP = "127.0.0.1";
    uint32_t port = 9554;
    if (argc > 1) {
        parseIPAndPort(argv[1], IP, port);
    }
    std::cout << "Working on: " << IP << ' ' << port << endl;


    std::cout << "Write your name: " << std::endl;

    std::string name;
    std::getline(std::cin, name, '\n');

    client.connectTo(IP, port);

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