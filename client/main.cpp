#include "UClient.h"

#include <string>

int main()
{
    UClient client;

    client.connectTo("127.0.0.1", 9554);
    client.joinThreads();
}