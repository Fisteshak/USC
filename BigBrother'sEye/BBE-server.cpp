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
#include <locale.h>
#include <ciso646>



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
    uint64_t usedVirtualMem = 0, totalVirtualMem = 0, usedPhysMem = 0, totalPhysMem = 0; //в КБ
    uint16_t procLoad = 0;   //в процентах

};

struct user{
    std::string name = "";
    std::string IP = "";
    UServer::Client* sock;
    SystemResInfo resInfo;
    bool operator==(const user &a) {
        return (a.sock == sock and a.name == name);
    }
    enum Status {
        connected, disconnected
    } status = disconnected;
};

enum Mode {
    computers,
    processes
};

Mode mode = computers;   //что сейчас выводится на экран
int compNum = 0;            //какой компьютер выводится на экран

std::list <user> users;
int nUsers = 0;

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

void printComputers()
{
    system("cls");

    int num = 1;

    fmt::print(fmt::emphasis::bold,
    "{:^3} {:<20} {:^15} {:>15} {:>15} {:>4} {:>10}\n",
    "№", "Имя компьютера", "IP", "Вирт. память", "Физ. память", "CPU", "Статус");

    fmt::print("{:=<3} {:=<20} {:=>15} {:=>15} {:=>15} {:=>4} {:=>10}\n", "", "", "", "", "", "", "");

    for (const auto& x : users) {

        fmt::print("{:2}. {:20} {:^15} {:>5.1f} / {:4.1f} ГБ {:>5.1f} / {:4.1f} ГБ {:3}% ",
            num++, x.name, x.IP,
            double(x.resInfo.usedVirtualMem) / (1024 * 1024), double(x.resInfo.totalVirtualMem) / (1024 * 1024),
            double(x.resInfo.usedPhysMem) / (1024 * 1024), double(x.resInfo.totalPhysMem) / (1024 * 1024),
            x.resInfo.procLoad);

        switch (x.status)
        {
        case user::Status::connected:
            fmt::print(fg(fmt::color::green), "{:>10}\n", "В СЕТИ");
            break;
        case user::Status::disconnected:
            fmt::print(fg(fmt::color::red), "{:>10}\n", "НЕ В СЕТИ");
            break;
        default:
            break;
        }

        //cout << x.name << endl;
    }
    return;
}

void data_handler(UServer::DataBuffer& data, UServer::Client& cl)
{

    user* uref = std::any_cast <user*> (cl.ref);
    if (uref->name == "") {
        for (const auto& x : data) {
            if (x == '-' or ('A' <= x and x <= 'Z') or ('a' <= x and x <= 'z') or ('0' <= x and x <= '9'))
                uref->name.push_back(x);
        }
        if (uref->name.empty()) {
            uref->name = "Безымянный";
        }

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
    if (!cl.ref.has_value()) return;
    user* uref = std::any_cast <user*> (cl.ref);

    cl.ref = nullptr;

    //удаление отсоединившегося
    auto x = find(users.begin(), users.end(), *uref);

    x->status = user::Status::disconnected;

    //users.erase(x);
    //nUsers--;

    printComputers();

    return;
}

void conn_handler(UServer::Client& cl)
{
    // user temp;

    // temp.name = "";
    // temp.IP = cl.getIPstr();
    // temp.sock = &cl;
    // temp.status = user::Status::connected;

//    users.push_back(temp);

    auto x = find_if(users.begin(), users.end(), [&](const user& a){return a.IP == cl.getIPstr();});


    if (x != users.end()) {
        if (x->status != user::Status::connected) {
            x->status = user::Status::connected;
            cl.ref = &(*x);
        }
        else {
            server.disconnect(cl);
        }
    }
    else {
        user temp;

        temp.name = "";
        temp.IP = cl.getIPstr();
        temp.sock = &cl;
        temp.status = user::Status::connected;

        users.push_back(temp);
        cl.ref = &users.back();
    }


    nUsers++;

    std::string s;
    s = "[server] Succesfully connected";
    cl.sendPacket(s);
    return;

}




int main(int argc, char *argv[])
{

    //std::setlocale(LC_ALL,"Russian");
    //std::locale::global(std::locale("POSIX"));
    //std::cout << "The default locale is " << std::locale().name() << '\n';

    #ifndef DEBUG
    std::cerr.setstate(std::ios_base::failbit);  //отключить вывод cerr
    #endif

    //установить IP и порт из аргументов
    if (argc > 1) {
        std::string IP_Port = argv[1];
        if (IP_Port.find(":") != string::npos) {
            try {
                server.setPort(stoul(IP_Port.substr(IP_Port.find(":")+1, IP_Port.size())));
            }
            catch (std::exception) {
                fmt::print("Неправильный порт\n");
                return 1;
            }
            server.setIP(IP_Port.substr(0, IP_Port.find(":")));
        }
        else {
            server.setIP(IP_Port.substr(0, IP_Port.find(":")));
        }
    }

    server.setDataHandler(data_handler);
    server.setDisconnHandler(disconn_handler);
    server.setConnHandler(conn_handler);
    server.run();

    if (server.getStatus() == UServer::status::up) fmt::print("[server] Сервер запущен\n");
    else {
        return 1;
    }

    std::string s;

    UServer::DataBufferStr buf;
    do {
        cin >> s;

    } while (s != ":stop");

    server.stop();
    std::cout << "[server] Сервер остановлен" << std::endl;
    return 0;
}
