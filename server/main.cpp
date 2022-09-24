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

int main()
{
    
    UServer server(9554, "127.0.0.1", 50);
    server.set_data_handler(data_handler);
    server.set_disconn_handler(disconn_handler);
    server.run();
    int x;
    std::cin >> x;
    server.stop();        
}
