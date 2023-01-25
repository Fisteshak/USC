#include "UServer.h"

#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>
#include <fstream>
#include <list>

//#define DEBUG

using namespace std;

UServer server("127.0.0.1", 9554, 50, UServer::CRYPTO_ENABLED);

struct user{
    std::string name = "";
    UServer::Client* sock;
    bool operator==(const user &a) {
        return (a.sock == sock and a.name == name);
    }
};

std::list <user> users;


int nUsers = 0;


void data_handler(UServer::DataBuffer& data, UServer::Client& cl)
{

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

        std::cout << uref->name << ": ";
        for (int i = 0; i < data.size(); i++) {
            cout << data[i];
        }
        cout << endl;

        UServer::DataBufferStr data2 = uref->name;
        data2 += ": ";

        for (const auto &x : data) {
            data2.push_back(x);
        }

        for (const auto& x : users) {
            if (*(x.sock) != cl)
                x.sock->sendPacket(data2);
        }
        return;
    }
}

void disconn_handler(UServer::Client& cl)
{
    user* uref = std::any_cast <user*> (cl.ref);

    std::cout << "[server] " << uref->name << " disconnected"  << std::endl;

    cl.ref = nullptr;

    //удаление отсоединившегося
    auto x = find(users.begin(), users.end(), *uref);
    users.erase(x);
    nUsers--;

    return;
}

void conn_handler(UServer::Client& cl)
{
    // users2[nUsers].name = "";
    // users2[nUsers].sock = &cl;
    users.push_back({"", &cl});
    cl.ref = &users.back();
    nUsers++;

    std::string s;
    s = "[server] Succesfully connected";
    cl.sendPacket(s);
    return;
}

int main()
{

    server.setDataHandler(data_handler);
    server.setDisconnHandler(disconn_handler);
    server.setConnHandler(conn_handler);
    server.run();

    std::cout << "[server] Server started working" << std::endl;

    std::string x;

    UServer::DataBufferStr buf;
    do {

        std::getline(std::cin, buf);

        server.sendPacket(buf);

    } while (buf != ":stop");
    server.stop();
    std::cout << "[server] Server stopped working" << std::endl;
    return 0;
}