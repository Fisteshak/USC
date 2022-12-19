#include "UClient.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <vector>
#include <cassert>
#include <chrono>

#include "windows.h"
#include <psapi.h>
#include <tlhelp32.h>


using namespace std;

struct process
{
    string exeName;
    uint32_t ID;
    uint64_t memoryUsage;
};

vector<process> processes;

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
    return {szProcessName, processID, pmc.WorkingSetSize};
}

void getProcessesPSAPI(vector<process> &processes)
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return;
    }

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);
    processes.resize(cProcesses);

    // Print the name and process identifier for each process.

    for (i = 0; i < cProcesses; i++)
    {
        if (aProcesses[i] != 0)
        {
            processes[i] = getProcessInfo(aProcesses[i]);
        }
    }
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
            processes[i].memoryUsage = pmc.PrivateUsage;
        }

        // имя + '\0' для разделения + ID + память
        size += processes[i].exeName.size() + 1 + 4 + 8;

        i++;

        // printf("ProcessID= %d\n", pe32.th32ProcessID);
        // printf("ExeFile= %s\n", pe32.szExeFile);
        // printf("Memory usage= %d\n\n", pmc.PrivateUsage);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return size;
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

int main()
{

    UClient client;

    client.setDataHandler(data_handler);
    client.setDisconnHandler(disconn_handler);
    client.setConnHandler(conn_handler);


    UClient::DataBuffer buf;


    client.connectTo("127.0.0.1", 9554);

    if (client.getStatus() != UClient::status::connected) {
        std::cout << "Failed to connect to a server\n";
        return 0;
    }

    string name = "task manager";
    client.sendPacket(name);

    uint32_t size;
    while (true) {
        processes.clear();
        size = getProcesses(processes);
        processesToBytes(processes, size, buf);
        processes.clear();
        bytesToProcesses(processes, buf);
        client.sendPacket(buf);

        for (const auto &x : processes) {
            std::cout << x.ID << "  " << x.exeName << "   " << x.memoryUsage << std::endl;
        }

        this_thread::sleep_for(5s);
    }



//    bytesToProcesses(processes, buf);


    // assert(processes.size() == processes2.size());
    // for (int i = 0; i < processes.size(); i++)
    // {
    //     assert(processes[i].exeName == processes2[i].exeName);
    //     assert(processes[i].ID == processes2[i].ID);
    //     assert(processes[i].memoryUsage == processes2[i].memoryUsage);
    // }
}