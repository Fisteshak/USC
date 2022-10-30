#include "UClient.h"
#include <iostream>
#include <string>

void conn_handler()
{
    std::cout << "Successfully connected to server" << std::endl;
}

void data_handler(UClient::DataBuffer& data)
{
    for (int i = 0; data[i] != '\0'; i++) {
        std::cout << data[i];
    }
    std::cout << std::endl;

}


int main()
{
    UClient client;

    client.set_conn_handler(conn_handler);
    client.set_data_handler(data_handler);

    client.connectTo("127.0.0.1", 9554);
    if (client.getStatus() != UClient::status::connected) {
        std::cout << "Failed to connect to a server\n";
        return 0;
    }


    std::string s = "";
    do {
        std::cin >> s;
    } while (s != ":stop");

    client.disconnect();

}