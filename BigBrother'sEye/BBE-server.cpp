#include "UServer.h"

#include <iomanip>
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>
#include <fstream>

//#define DEBUG

using namespace std;

UServer server("127.0.0.1", 9554, 50);

struct user{
    std::string name = "";
    UServer::Client* sock;
};

std::vector <user> users;
int nUsers = 0;

struct process
{
    string exeName;
    uint32_t ID;
    uint64_t memoryUsage;
};

struct SystemResInfo {
    vector <process> procs;
    uint64_t usedVirtualMem, totalVirtualMem, usedPhysMem, totalPhysMem;
};

//j - порядковый норер байта, куда начнется запись байт
void bytesToProcesses(vector<process> &processes, const vector<char> &data, uint32_t &j)
{
    uint32_t procNum = 0;
    memcpy(&procNum, data.data(), sizeof(procNum));
    j += sizeof(procNum);

    processes.resize(procNum);
    for (int i = 0; i < procNum; i++)
    {
        processes[i].exeName = (string)(data.data() + j);
        j += processes[i].exeName.size() + 1;

        memcpy(&processes[i].ID, data.data() + j, sizeof(processes[i].ID));
        j += sizeof(processes[i].ID);

        memcpy(&processes[i].memoryUsage, data.data() + j, sizeof(processes[i].memoryUsage));
        j += sizeof(processes[i].memoryUsage);
    }
    return;

}

//j - порядковый норер байта, куда начнется запись байт
void bytesToResInfo(SystemResInfo& resInfo, const vector <char>& data, uint32_t& j)
{
    bytesToProcesses(resInfo.procs, data, j);

    memcpy(&resInfo.usedVirtualMem, data.data() + j, sizeof(resInfo.usedVirtualMem));
    j += sizeof(resInfo.usedVirtualMem);
    memcpy(&resInfo.totalVirtualMem, data.data() + j, sizeof(resInfo.totalVirtualMem));
    j += sizeof(resInfo.totalVirtualMem);
    memcpy(&resInfo.usedPhysMem, data.data() + j, sizeof(resInfo.usedPhysMem));
    j += sizeof(resInfo.usedPhysMem);
    memcpy(&resInfo.totalPhysMem, data.data() + j, sizeof(resInfo.totalPhysMem));
    j += sizeof(resInfo.totalPhysMem);

}



vector <process> processes;
SystemResInfo resInfo;

void data_handler(UServer::DataBuffer& data, UServer::Client& cl)
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
        processes.clear();
        uint32_t j = 0;
        bytesToResInfo(resInfo, data, j);
        for (const auto &x : resInfo.procs) {
            std::cout << x.ID << "  " << x.exeName << "   " << x.memoryUsage << std::endl;
        }

        cout << setprecision(3);
        cout << "Physical memory used: " << double(resInfo.usedPhysMem) / (1024 * 1024)
        << " / " << double(resInfo.totalPhysMem) / (1024 * 1024)  << "GB" << endl;

        cout << "Virtual memory used: " << double(resInfo.usedVirtualMem) / (1024 * 1024)
        << " / " << double(resInfo.totalVirtualMem) / (1024 * 1024) << "GB" << endl;

    }
    return;
}

void disconn_handler(UServer::Client& cl)
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

void conn_handler(UServer::Client& cl)
{
    users[nUsers].name = "";
    users[nUsers].sock = &cl;
    cl.ref = &users[nUsers];
    nUsers++;

    std::string s;
    s = "[server] Succesfully connected";
    cl.sendPacket(s);
    return;
}


int main()
{

    #ifndef DEBUG
    std::cerr.setstate(std::ios_base::failbit);  //отключить вывод cerr
    #endif

    server.setDataHandler(data_handler);
    server.setDisconnHandler(disconn_handler);
    server.setConnHandler(conn_handler);
    server.run();

    std::cout << "[server] Server started working" << std::endl;

    users.resize(50);
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