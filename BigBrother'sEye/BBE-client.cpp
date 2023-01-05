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
#include <fmt/format.h>

#include "TCHAR.h"
#include "pdh.h"
#include "windows.h"
#include <psapi.h>
#include <tlhelp32.h>

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
    //std::cout << data.data() << std::endl;
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

    // Print the process name and identifier.

    //_tprintf(TEXT("%s  (PID: %u)  %u\n"), szProcessName, processID, pmc.WorkingSetSize / 1024);

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

        // printf("ProcessID= %d\n", pe32.th32ProcessID);
        // printf("ExeFile= %s\n", pe32.szExeFile);
        // printf("Memory usage= %d\n\n", pmc.PrivateUsage);
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

void bytesToProcesses(vector<process> &processes, const vector<char> &data)
{
    uint32_t procNum = 0;
    memcpy(&procNum, data.data(), sizeof(procNum));
    int j = sizeof(procNum);

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

void processesToBytes(const vector<process> &processes, uint32_t size, vector<char> &data)
{
    data.resize(size + 4, '\0');
    // количество процессов
    uint32_t procNum = processes.size();
    memcpy(data.data(), &procNum, sizeof(procNum));
    int j = sizeof(procNum);

    // to bytes
    for (int i = 0; i < processes.size(); i++)
    {
        processes[i].exeName.copy(data.data() + j, processes[i].exeName.size(), 0);

        j += processes[i].exeName.size();
        data[j] = '\0';
        j++;

        memcpy(data.data() + j, &processes[i].ID, sizeof(processes[i].ID));
        j += sizeof(processes[i].ID);

        memcpy(data.data() + j, &processes[i].memoryUsage, sizeof(processes[i].memoryUsage));
        j += sizeof(processes[i].memoryUsage);

    }
    return;
}

//prSize - размер массива процессов
void resInfoToBytes(const SystemResInfo& resInfo, const uint32_t prSize, vector <char>& data)
{
    processesToBytes(resInfo.procs, prSize, data);

    int j = data.size();

    data.resize(data.size() + sizeof(resInfo.usedVirtualMem));
    memcpy(data.data() + j, &resInfo.usedVirtualMem, sizeof(resInfo.usedVirtualMem));
    j += sizeof(resInfo.usedVirtualMem);

    data.resize(data.size() + sizeof(resInfo.totalVirtualMem));
    memcpy(data.data() + j, &resInfo.totalVirtualMem, sizeof(resInfo.totalVirtualMem));
    j += sizeof(resInfo.totalVirtualMem);

    data.resize(data.size() + sizeof(resInfo.usedPhysMem));
    memcpy(data.data() + j, &resInfo.usedPhysMem, sizeof(resInfo.usedPhysMem));
    j += sizeof(resInfo.usedPhysMem);

    data.resize(data.size() + sizeof(resInfo.totalVirtualMem));
    memcpy(data.data() + j, &resInfo.totalPhysMem, sizeof(resInfo.totalPhysMem));
    j += sizeof(resInfo.totalPhysMem);

    data.resize(data.size() + sizeof(resInfo.procLoad));
    memcpy(data.data() + j, &resInfo.procLoad, sizeof(resInfo.procLoad));
    j += sizeof(resInfo.procLoad);

    return;
}


int main()
{

    UClient client;

    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);

    UClient::DataBuffer buf;

    client.connectTo("127.0.0.1", 9554);

    // if (client.getStatus() != UClient::status::connected) {
    //     std::cout << "Failed to connect to a server\n";
    //     return 0;
    // }

    string name = "task manager";
    client.sendPacket(name);
    initPdh();
    uint32_t size;
    SystemResInfo resInfo;
    while (true) {
        resInfo.procs.clear();
        size = getProcesses(resInfo.procs);

        resInfo.usedPhysMem = getUsedPhysMem();
        resInfo.totalPhysMem = getTotalPhysMem();
        resInfo.usedVirtualMem = getUsedVirtMem();
        resInfo.totalVirtualMem = getTotalVirtMem();
        resInfo.procLoad = uint16_t(getProcessorLoad());

        resInfoToBytes(resInfo, size, buf);

        client.sendPacket(buf);

        for (const auto &x : resInfo.procs) {
            fmt::print("ID: {:<10} Name: {:<40} Mem: {:8.1f} MB\n", x.ID, x.exeName, double(x.memoryUsage) / 1024);
        }

        fmt::print("Physical memory used: {:.3} / {:.3} GB\n",
         double(resInfo.usedPhysMem) / (1024 * 1024), double(resInfo.usedPhysMem) / (1024 * 1024));
        fmt::print("Virtual memory used: {:.3} / {:.3} GB\n",
         double(resInfo.usedVirtualMem) / (1024 * 1024), double(resInfo.totalVirtualMem) / (1024 * 1024));
        fmt::print("Processor load: {}%\n", resInfo.procLoad);



        // for (const auto &x : resInfo.procs) {
        //     std::cout << x.ID << "  " << x.exeName << "   " << x.memoryUsage / 1024.0 << std::endl;
        // }
        // cout << setprecision(3) << fixed;
        // cout << "Physical memory used: " << double(resInfo.usedPhysMem) / (1024 * 1024)
        // << " / " << double(resInfo.totalPhysMem) / (1024 * 1024)  << "GB" << endl;

        // cout << "Virtual memory used: " << double(resInfo.usedVirtualMem) / (1024 * 1024)
        // << " / " << double(resInfo.totalVirtualMem) / (1024 * 1024) << "GB" << endl;

        // cout << "Processor load: " << resInfo.procLoad << "%" << endl;

        this_thread::sleep_for(2.5s);
    }

}