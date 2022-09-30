#include "UServer.h"
#include <memory>
#include <thread>
#include <iostream>

UServer server(9554, "127.0.0.1", 50);

void data_handler(UServer::data_buffer_t& data, UServer::client& cl)
{    
    std::cout << cl.fd << ": " << data.data() << std::endl;
    server.sendData(data);    
    return;
}

void disconn_handler(UServer::client& cl)
{
    std::cout << "Client " << cl.fd << " has disconnected" << std::endl;
    return;
}

void conn_handler(UServer::client& cl)
{
    std::cout << "Client " << cl.fd << " has connected" << std::endl;
    return;
}

int main()
{
    
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
    return 0;
}


