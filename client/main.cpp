#include "UClient.h"
#include <iostream>
#include <string>

int main()
{
    UClient client;

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