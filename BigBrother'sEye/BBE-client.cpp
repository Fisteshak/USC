#include "UClient.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <vector>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <cmath>

#include "TCHAR.h"
#include "pdh.h"
#include "windows.h"
#include <psapi.h>
#include <tlhelp32.h>

#include <fmt/format.h>
#include <alpaca/alpaca.h>

using namespace std;

struct process
{
    string exeName;
    uint32_t ID;
    uint64_t memoryUsage;  //в КБ
};

struct SystemResInfo {
    vector <process> procs;
    uint64_t usedVirtualMem, totalVirtualMem, usedPhysMem, totalPhysMem;   //в КБ
    uint16_t procLoad;   //в процентах
};


static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;


void data_handler(UClient::DataBuffer& data)
{

    for (int i = 0; i < data.size(); i++) {
        std::cout << data[i];
    }
    cout << endl;
}

void conn_handler()
{
    std::cout << "[client] Succesfully connected to server" << std::endl;
}

void disconn_handler()
{
    std::cout << "[client] Disconnected from server" << std::endl;
}



void initPdh(){
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    // You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with PdhGetFormattedCounterArray()
    PdhAddEnglishCounter(cpuQuery, TEXT("\\Processor(_Total)\\% Processor Time"), NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
}

double getProcessorLoad(){
    PDH_FMT_COUNTERVALUE counterVal;

    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    return counterVal.doubleValue;
}


process getProcessInfo(const DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                                  FALSE, processID);

    // Get the process name.
    PROCESS_MEMORY_COUNTERS_EX pmc;

    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod),
                                 &cbNeeded, 3))
        {
            GetModuleBaseName(hProcess, hMod, szProcessName,
                              sizeof(szProcessName) / sizeof(TCHAR));
            GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
        }
    }


    // Release the handle to the process.

    CloseHandle(hProcess);
    return {szProcessName, processID, pmc.WorkingSetSize / 1024};
}





// возвращает полный размер вектора с процессами
int32_t getProcesses(vector<process> &processes)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    PROCESS_MEMORY_COUNTERS_EX pmc;

    // сделать снимок системы
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return -1;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    // получить первый процесс
    if (!Process32First(hProcessSnap, &pe32))
    {
        cout << "Process32First Error" << endl;
        CloseHandle(hProcessSnap); // clean the snapshot object
        return -1;
    }

    int i = 0;
    long long size = 0;
    process empty;

    // перебрать процессы
    do
    {
        if (processes.size() <= i)
        {
            processes.push_back(empty);
        }

        processes[i].exeName = pe32.szExeFile;
        processes[i].ID = pe32.th32ProcessID;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                      FALSE, pe32.th32ProcessID);

        if (hProcess == NULL)
        {
            processes[i].memoryUsage = 0;
        }
        else
        {
            GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc));
            processes[i].memoryUsage = pmc.PrivateUsage / 1024;
        }

        // имя + '\0' для разделения + ID + память
        size += processes[i].exeName.size() + 1 + sizeof(processes[i].ID) + sizeof(processes[i].memoryUsage);

        i++;

    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return size;
}

//возвращает в КБ
uint64_t getTotalPhysMem()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return (memInfo.ullTotalPhys / 1024);
}

//возвращает в КБ
uint64_t getUsedPhysMem()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / 1024;
}

//возвращает в КБ
uint64_t getTotalVirtMem()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return (memInfo.ullTotalPageFile / 1024);
}

//возвращает в КБ
uint64_t getUsedVirtMem()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return (memInfo.ullTotalPageFile - memInfo.ullAvailPageFile) / 1024;
}

string GetComputerName()
{
    char buffer[MAX_COMPUTERNAME_LENGTH + 1] = "";
    unsigned long size = MAX_COMPUTERNAME_LENGTH + 1;
    ::GetComputerName(buffer, &size);

    return string(buffer);
}

int main(int argc, char *argv[])
{
    UClient client;

    //IP и порт по умолчанию
    uint32_t port = 9554;
    std::string IP = "127.0.0.1";

    //установить IP и порт из аргументов
    if (argc > 1) {
        std::string IP_Port = argv[1];
        if (IP_Port.find(":") != string::npos) {
            try {
                port = stoul(IP_Port.substr(IP_Port.find(":")+1, IP_Port.size()));
            }
            catch (std::exception) {
                fmt::print("Неправильный порт\n");
                return 1;
            }
            IP = IP_Port.substr(0, IP_Port.find(":"));
        }
        else {
            IP = IP_Port.substr(0, IP_Port.find(":"));
        }
    }


    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);

    UClient::DataBuffer buf;

    client.connectTo(IP, port);

    if (client.getStatus() != UClient::status::connected) {
        std::cout << "Failed to connect to a server\n";
        return 0;
    }

    string name = GetComputerName();
    client.sendPacket(name);
    initPdh();
    uint32_t size;
    SystemResInfo resInfo;
    while (true) {
        resInfo.procs.clear();
        getProcesses(resInfo.procs);

        resInfo.usedPhysMem = getUsedPhysMem();
        resInfo.totalPhysMem = getTotalPhysMem();
        resInfo.usedVirtualMem = getUsedVirtMem();
        resInfo.totalVirtualMem = getTotalVirtMem();
        resInfo.procLoad = uint16_t(getProcessorLoad());

        buf.clear();
        alpaca::serialize(resInfo, buf);

        client.sendPacket(buf);

        // for (const auto &x : resInfo.procs) {
        //     fmt::print("ID: {:<10} Name: {:<40} Mem: {:8.1f} MB\n", x.ID, x.exeName, double(x.memoryUsage) / 1024);
        // }
        system("cls");
        fmt::print("Physical memory used: {:.3} / {:.3} GB\n",
         double(resInfo.usedPhysMem) / (1024 * 1024), double(resInfo.usedPhysMem) / (1024 * 1024));
        fmt::print("Virtual memory used: {:.3} / {:.3} GB\n",
         double(resInfo.usedVirtualMem) / (1024 * 1024), double(resInfo.totalVirtualMem) / (1024 * 1024));
        fmt::print("Processor load: {}%\n", resInfo.procLoad);

        this_thread::sleep_for(2.5s);
    }

}