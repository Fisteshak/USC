#include "UServer.h"
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
UServer server(9554, "127.0.0.1", 50);

struct user{
    std::string name = "";
    UServer::client& sock;
};

std::vector <user> users;
int nUsers = 0;


void data_handler(UServer::data_buffer_t& data, UServer::client& cl)
{
    user* uref = (user*)cl.ref;
    if (uref->name == "") {
        int i = 0;
        while (data[i] != '\0'){
            uref->name.push_back(data[i]);
            i++;
        }
        std::cout << uref->name << ": connected\n";
    }
    else {
        std::cout << uref->name << ": " << data.data() << std::endl;
        server.sendData(data);
        return;
    }
}

void disconn_handler(UServer::client& cl)
{
    user* uref = (user*)cl.ref;
    std::cout << uref->name << " disconnected"  << std::endl;
    uref->name = "";
    cl.ref = nullptr;

    return;
}

void conn_handler(UServer::client& cl)
{
    users.push_back({"", cl});
    cl.ref = &users.back();
    //user* uref = (user*)cl.ref;
    //std::cout << uref->name << " connected"  << std::endl;
    return;
}


int main()
{

    server.set_data_handler(data_handler);
    server.set_disconn_handler(disconn_handler);
    server.set_conn_handler(conn_handler);
    server.run();
    users.reserve(50);
    std::string x;
    do {
        std::cin >> x;
        UServer::data_buffer_t buf;

        for (auto c : x) {
            buf.push_back(c);
        }
        server.sendData(buf);
    } while (x != ":stop");
    server.stop();
    std::cout << "Server has stopped working" << std::endl;
    return 0;
}


