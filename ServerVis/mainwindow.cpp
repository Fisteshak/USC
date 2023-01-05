#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "UServer.h"

#include <iomanip>
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <any>
#include <fstream>
#include <list>
#include <QTextStream>
#include <QDebug>

using namespace std;

UServer server("127.0.0.1", 9554, 50);

struct user{
    std::string name = "";
    UServer::Client* sock;
    bool operator==(const user &a) {
        return (a.sock == sock and a.name == name);
    }
};

std::list <user> users;
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
    uint16_t procLoad;   //в процентах

};

//j - порядковый норер байта, куда начнется запись байт
void bytesToProcesses(vector<process> &processes, const vector<char> &data, uint32_t &j)
{
    uint32_t procNum = 0;
    memcpy(&procNum, data.data(), sizeof(procNum));
    j += sizeof(procNum);

    processes.resize(procNum);
    for (uint32_t i = 0; i < procNum; i++)
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
    memcpy(&resInfo.procLoad, data.data() + j, sizeof(resInfo.procLoad));
    j += sizeof(resInfo.procLoad);

    return;
}

SystemResInfo resInfo;

void data_handler(UServer::DataBuffer& data, UServer::Client& cl)
{

    //user* uref = (user*)cl.ref;
    user* uref = std::any_cast <user*> (cl.ref);
    if (uref->name == "") {
        for (const auto& x : data) {
            uref->name.push_back(x);
        }

        // int i = 0;
        // while (data[i] != '\0'){
        //     uref->name.push_back(data[i]);
        //     i++;
        // }
        std::cout << "[server] " << uref->name << " connected\n";
    }
    else {
        resInfo.procs.clear();
        uint32_t j = 0;
        bytesToResInfo(resInfo, data, j);

        for (const auto &x : resInfo.procs) {
            std::cout << x.ID << "  " << x.exeName << "   " << x.memoryUsage << std::endl;
        }

        Ui::MainWindow::textEdit
        std::cout << std::setprecision(3);
        std::cout << "Physical memory used: " << double(resInfo.usedPhysMem) / (1024 * 1024)
        << " / " << double(resInfo.totalPhysMem) / (1024 * 1024)  << "GB" << endl;

        std::cout << "Virtual memory used: " << double(resInfo.usedVirtualMem) / (1024 * 1024)
        << " / " << double(resInfo.totalVirtualMem) / (1024 * 1024) << "GB" << endl;

        std::cout << "Processor load: " << resInfo.procLoad << "%" << endl;

    }
    return;
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



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    server.setDataHandler(data_handler);
    server.setDisconnHandler(disconn_handler);
    server.setConnHandler(conn_handler);
    server.run();
    //std::cout.rdbuf(ui->textEdit->text);
}

MainWindow::~MainWindow()
{
    delete ui;
    server.stop();
}

