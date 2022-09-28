#include "UServer.h"
#include <memory>
#include <thread>
#include <iostream>

void data_handler(UServer::data_buffer_t& data)
{
    for (int i = 0; i < 20; i++) 
        std::cout << data[i];
    std::cout << std::endl;
    return;
}

void disconn_handler()
{
    std::cout << "Client has disconnected" << std::endl;
}

void conn_handler()
{
    std::cout << "Client has connected" << std::endl;
}

int main()
{
    UServer server(9554, "127.0.0.1", 50);
    server.set_data_handler(data_handler);
    server.set_disconn_handler(disconn_handler);
    server.set_conn_handler(conn_handler);
    server.run();
    std::string x;
    do {        
        std::cin >> x;        
    } while (x != ":stop");
    server.stop();
    std::cout << "Server has stopped working" << std::endl;
}
