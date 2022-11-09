#include "UServer.h"

#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>

UServer server(9554, "127.0.0.1", 50);

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
        std::cout << uref->name << " connected\n";
    }
    else {
        std::cout << uref->name << ": " << data.data() << std::endl;
        for (int i = 0; i < nUsers; i++) {
            if (*(users[i].sock) != cl) {
                users[i].sock->sendData(data);
            }
        }
        return;
    }
}

void disconn_handler(UServer::client& cl)
{
    user* uref = std::any_cast <user*> (cl.ref);

    std::cout << uref->name << " disconnected"  << std::endl;

    uref->name = "";
    cl.ref.reset();

    for (int i = 0; i < nUsers; i++) {
        if (*(users[nUsers].sock) == cl) {
            users[i] = users[nUsers];
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
    nUsers++;
    cl.ref = &users[nUsers];

    std::string s;
    s = "succesfully connected";
    cl.sendData(s);
    return;
}


int main()
{

    server.set_data_handler(data_handler);
    server.set_disconn_handler(disconn_handler);
    server.set_conn_handler(conn_handler);
    server.run();
    users.resize(50);
    std::string x;



    UServer::DataBufferStr buf;
    do {

        std::getline(std::cin, buf, '\n');

        server.sendData(buf);

    } while (buf != ":stop");
    server.stop();
    std::cout << "Server has stopped working" << std::endl;
    return 0;
}