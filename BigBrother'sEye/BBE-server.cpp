#include "UServer.h"

#include <iomanip>
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>
#include <fstream>
#include <list>
#include <string>

#include <fmt/format.h>
#include <fmt/color.h>
#include <nlohmann/json.hpp>
#include <alpaca/alpaca.h>


//#define DEBUG

using namespace std;

UServer server("127.0.0.1", 9554, 50);

struct process
{
    string exeName;
    uint32_t ID;
    uint64_t memoryUsage; //в КБ
};

struct SystemResInfo {
    vector <process> procs;
    uint64_t usedVirtualMem, totalVirtualMem, usedPhysMem, totalPhysMem; //в КБ
    uint16_t procLoad;   //в процентах

};

struct user{
    std::string name = "";
    UServer::Client* sock;
    SystemResInfo resInfo;
    bool operator==(const user &a) {
        return (a.sock == sock and a.name == name);
    }
};

enum Mode {
    computers,
    processes
};

Mode mode = computers;   //что сейчас выводится на экран
int compNum = 0;            //какой компьютер выводится на экран

std::list <user> users;
int nUsers = 0;


// //j - порядковый норер байта, куда начнется запись байт
// void bytesToProcesses(vector<process> &processes, const vector<unsigned char> &data, uint32_t &j)
// {
//     uint32_t procNum = 0;
//     memcpy(&procNum, data.data(), sizeof(procNum));
//     j += sizeof(procNum);

//     processes.resize(procNum);
//     for (uint32_t i = 0; i < procNum; i++)
//     {
//         processes[i].exeName = (string)(data.data() + j);
//         j += processes[i].exeName.size() + 1;

//         memcpy(&processes[i].ID, data.data() + j, sizeof(processes[i].ID));
//         j += sizeof(processes[i].ID);

//         memcpy(&processes[i].memoryUsage, data.data() + j, sizeof(processes[i].memoryUsage));
//         j += sizeof(processes[i].memoryUsage);
//     }
//     return;

// }

// //j - порядковый норер байта, куда начнется запись байт
// void bytesToResInfo(SystemResInfo& resInfo, const vector <unsigned char>& data, uint32_t& j)
// {
//     bytesToProcesses(resInfo.procs, data, j);

//     memcpy(&resInfo.usedVirtualMem, data.data() + j, sizeof(resInfo.usedVirtualMem));
//     j += sizeof(resInfo.usedVirtualMem);
//     memcpy(&resInfo.totalVirtualMem, data.data() + j, sizeof(resInfo.totalVirtualMem));
//     j += sizeof(resInfo.totalVirtualMem);
//     memcpy(&resInfo.usedPhysMem, data.data() + j, sizeof(resInfo.usedPhysMem));
//     j += sizeof(resInfo.usedPhysMem);
//     memcpy(&resInfo.totalPhysMem, data.data() + j, sizeof(resInfo.totalPhysMem));
//     j += sizeof(resInfo.totalPhysMem);
//     memcpy(&resInfo.procLoad, data.data() + j, sizeof(resInfo.procLoad));
//     j += sizeof(resInfo.procLoad);

//     return;
// }


void printComputers()
{
    system("cls");

    int num = 1;

    fmt::print(fmt::emphasis::bold,
    "{:^3} {:<20} {:>15} {:>15} {:>4}\n",
    "№", "Имя компьютера", "Вирт. память", "Физ. память", "CPU");

    fmt::print("{:=<3} {:=<20} {:=>15} {:=>15} {:=>4}\n", "", "", "", "", "");

    for (const auto& x : users) {

        fmt::print("{:2}. {:20} {:>5.1f} / {:4.1f} ГБ {:>5.1f} / {:4.1f} ГБ {:3}%\n",
        num++, x.name,
        double(x.resInfo.usedVirtualMem) / (1024 * 1024), double(x.resInfo.totalVirtualMem) / (1024 * 1024),
        double(x.resInfo.usedPhysMem) / (1024 * 1024), double(x.resInfo.totalPhysMem) / (1024 * 1024),
        x.resInfo.procLoad);
    }
    return;
}

void data_handler(UServer::DataBuffer& data, UServer::Client& cl)
{

    user* uref = std::any_cast <user*> (cl.ref);
    if (uref->name == "") {
        for (const auto& x : data) {
            uref->name.push_back(x);
        }
        system("cls");

        printComputers();

    }
    else {
        uref->resInfo.procs.clear();
        uint32_t j = 0;
        //bytesToResInfo(uref->resInfo, data, j);
        std::error_code ec;
        uref->resInfo = alpaca::deserialize<SystemResInfo>(data, ec);

        // for (const auto &x : uref->resInfo.procs) {
        //     fmt::print("ID: {:<10} Name: {:<40} Mem: {:8.1f} MB\n", x.ID, x.exeName, double(x.memoryUsage) / 1024);
        // }

        // fmt::print("Physical memory used: {:.3} / {:.3} GB\n",
        //  double(uref->resInfo.usedPhysMem) / (1024 * 1024), double(uref->resInfo.usedPhysMem) / (1024 * 1024));
        // fmt::print("Virtual memory used: {:.3} / {:.3} GB\n",
        //  double(uref->resInfo.usedVirtualMem) / (1024 * 1024), double(uref->resInfo.totalVirtualMem) / (1024 * 1024));
        // fmt::print("Processor load: {}%\n", uref->resInfo.procLoad);
        //printProcesses();
        printComputers();


    }
    return;
}

void disconn_handler(UServer::Client& cl)
{
    user* uref = std::any_cast <user*> (cl.ref);

    cl.ref = nullptr;

    //удаление отсоединившегося
    auto x = find(users.begin(), users.end(), *uref);
    users.erase(x);
    nUsers--;

    printComputers();

    return;
}

void conn_handler(UServer::Client& cl)
{
    users.push_back({"", &cl});
    cl.ref = &users.back();
    nUsers++;

    std::string s;
    s = "[server] Succesfully connected";
    cl.sendPacket(s);
    return;

}

void printProcesses(uint32_t num)
{
    auto user = users.begin();
    num--;
    while (num--) {
        user++;
    }

    for (const auto& x : user->resInfo.procs) {
        fmt::print("ID: {:<10} Name: {:<40} Mem: {:8.1f} MB\n", x.ID, x.exeName, double(x.memoryUsage) / 1024);
    }

    fmt::print("Physical memory used: {:.3} / {:.3} GB\n",
        double(user->resInfo.usedPhysMem) / (1024 * 1024), double(user->resInfo.usedPhysMem) / (1024 * 1024));
    fmt::print("Virtual memory used: {:.3} / {:.3} GB\n",
        double(user->resInfo.usedVirtualMem) / (1024 * 1024), double(user->resInfo.totalVirtualMem) / (1024 * 1024));
    fmt::print("Processor load: {}%\n", user->resInfo.procLoad);
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

    fmt::print("[server] Server started working\n");

    std::string s;

    UServer::DataBufferStr buf;
    do {
        cin >> s;

    } while (s != ":stop");

    server.stop();
    std::cout << "[server] Server stopped working" << std::endl;
    return 0;
}
