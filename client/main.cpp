#include "UClient.h"
#include <iostream>
#include <string>

int main()
{
    UClient client;

    client.connectTo("127.0.0.1", 9554);

    std::string s = "";
    do {
        std::cin >> s;
    } while (s != ":stop");

    client.disconnect();

}