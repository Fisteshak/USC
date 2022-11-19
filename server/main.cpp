#include "UServer.h"

#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>
#include <fstream>

//#define DEBUG

UServer server("127.0.0.1", 9554, 50);

struct user{
    std::string name = "";
    UServer::client* sock;
};


std::vector <user> users;
int nUsers = 0;


void data_handler(UServer::DataBuffer& data, UServer::client& cl)
{

    //user* uref = (user*)cl.ref;
    user* uref = std::any_cast <user*> (cl.ref);
    if (uref->name == "") {
        int i = 0;
        while (data[i] != '\0'){
            uref->name.push_back(data[i]);
            i++;
        }
        std::cout << "[server] " << uref->name << " connected\n";
    }
    else {

        std::cout << uref->name << ": " << data.data() << std::endl;
        UServer::DataBufferStr data2 = uref->name;
        data2 += ": ";
        data2 += data.data();


        for (int i = 0; i < nUsers; i++) {
            if (*(users[i].sock) != cl) {
                users[i].sock->sendData(data2);
            }
        }
        return;
    }
}

void disconn_handler(UServer::client& cl)
{
    user* uref = std::any_cast <user*> (cl.ref);

    std::cout << "[server] " << uref->name << " disconnected"  << std::endl;

    for (int i = 0; i < server.nConnections-1 || i < nUsers; i++) {
        std::cerr << server.clients[i+1].getSocket() << ' ';
        std::cerr << users[i].name << ' ' << users[i].sock << std::endl;
    }


    cl.ref = nullptr;


    for (int i = 0; i < nUsers; i++) {
        if (*(users[i].sock) == cl) {
            users[i] = users[nUsers-1];
            users[i].sock->ref = &users[i];
            break;
        }
    }
    nUsers--;


    return;
}

void conn_handler(UServer::client& cl)
{
    users[nUsers].name = "";
    users[nUsers].sock = &cl;
    cl.ref = &users[nUsers];
    nUsers++;

    std::string s;
    s = "[server] Succesfully connected";
    cl.sendData(s);
    return;
}


int main()
{

    #ifndef DEBUG
    std::cerr.setstate(std::ios_base::failbit);  //отключить вывод cerr
    #endif

    server.set_data_handler(data_handler);
    server.set_disconn_handler(disconn_handler);
    server.set_conn_handler(conn_handler);
    server.run();

    std::cout << "[server] Server started working" << std::endl;

    users.resize(50);
    std::string x;

    UServer::DataBufferStr buf;
    do {

        std::getline(std::cin, buf, '\n');

        server.sendData(buf);

    } while (buf != ":stop");
    server.stop();
    std::cout << "[server] Server stopped working" << std::endl;
    return 0;
}