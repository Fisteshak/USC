#include "UClient.h"
#include <fstream>
#include <iostream>
#include <string>
#include <tlhelp32.h>
using namespace std;

void data_handler(UClient::DataBuffer& data)
{

    // for (int i = 0; i < data.size(); i++) {
    //     std::cout << data[i];
    // }
    std::cout << data.data() << std::endl;
}

void conn_handler()
{
    std::cout << "[client] Succesfully connected to server" << std::endl;
}

void disconn_handler()
{
    std::cout << "[client] Disconnected from server" << std::endl;
}

struct process {
    string exeName;
    int ID;
};

vector<process> processes;
vector<process> processes2;

int getProcesses(vector<process> &processes)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    //сделать снимок системы
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return -1;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    process empty;

    //получить первый процесс
    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        cout << "Process32First Error" << endl;
        CloseHandle( hProcessSnap );          // clean the snapshot object
        return -1;
    }
    int i = 0;
    int size = 0;
    do  {
        // printf("pe32.th32ProcessID= %d\n", pe32.th32ProcessID);
        // printf("pe32.th32ParentProcessID= %d\n", pe32.th32ParentProcessID);
        // printf("pe32.szExeFile= %s\n\n", pe32.szExeFile);

        if (processes.size() <= i) {
            processes.push_back(empty);
        }
        processes[i].exeName = pe32.szExeFile;
        processes[i].ID = pe32.th32ProcessID;
        size += processes[i].exeName.size() + 4;
        i++;
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return size;
}

int main()
{
    //setlocale(LC_ALL, ".866");
    UClient client;
    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);

    // std::cout << "Write your name: " << std::endl;

    std::string name;
    // std::getline(std::cin, name, '\n');

    // client.connectTo("127.0.0.1", 9554);

    // client.sendPacket(name);

    // if (client.getStatus() != UClient::status::connected) {
    //     std::cout << "Failed to connect to a server\n";
    //     return 0;
    // }

    UClient::DataBufferStr data;
    int size = getProcesses(processes);
    data.resize(size);
    memcpy(data.data(), processes.data(), size);
    processes2.resize(processes.size());
    memcpy(processes2.data(), data.data(), size);

    for (const auto &x : processes2) {
        cout << x.exeName << "    " << x.ID << '\n';
    }



    // while (true) {

    //     std::getline(std::cin, data, '\n');

    //     if (data == ":stop" || client.getStatus() != UClient::status::connected) {
    //         break;
    //     }
    //     client.sendPacket(data);
    // }

    //client.disconnect();
}