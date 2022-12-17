#include "windows.h"
#include <fstream>
#include <iostream>
#include <psapi.h>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <tlhelp32.h>
#include <vector>

using namespace std;

struct process {
    string exeName;
    uint32_t ID;
    uint64_t memoryUsage;
};

vector<process> processes;
vector<process> processes2;

process getProcessInfo(const DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE, processID);

    // Get the process name.
    PROCESS_MEMORY_COUNTERS_EX pmc;

    if (NULL != hProcess) {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod),
                &cbNeeded, 3)) {
            GetModuleBaseName(hProcess, hMod, szProcessName,
                sizeof(szProcessName) / sizeof(TCHAR));
            GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        }
    }

    // Print the process name and identifier.

    //_tprintf(TEXT("%s  (PID: %u)  %u\n"), szProcessName, processID, pmc.WorkingSetSize / 1024);

    // Release the handle to the process.

    CloseHandle(hProcess);
    return { szProcessName, processID, pmc.WorkingSetSize };
}

void getProcessesPSAPI(vector<process>& processes)
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return;
    }

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);
    processes.resize(cProcesses);

    // Print the name and process identifier for each process.

    for (i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            processes[i] = getProcessInfo(aProcesses[i]);
        }
    }
}


//возвращает полный размер вектора с процессами
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
    if (!Process32First(hProcessSnap, &pe32)) {
        cout << "Process32First Error" << endl;
        CloseHandle(hProcessSnap); // clean the snapshot object
        return -1;
    }

    int i = 0;
    long long size = 0;
    process empty;

    //перебрать процессы
    do {
        if (processes.size() <= i) {
            processes.push_back(empty);
        }

        processes[i].exeName = pe32.szExeFile;
        processes[i].ID = pe32.th32ProcessID;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE, pe32.th32ProcessID);

        if (hProcess == NULL) {
            processes[i].memoryUsage = 0;
        }
        else {
            GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
            processes[i].memoryUsage = pmc.PrivateUsage;
        }

        size += processes[i].exeName.size() + 4 + 8; //имя + ID + память

        i++;

        // printf("ProcessID= %d\n", pe32.th32ProcessID);
        // printf("ExeFile= %s\n", pe32.szExeFile);
        // printf("Memory usage= %d\n\n", pmc.PrivateUsage);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return size;
}

int main()
{

    getProcesses(processes);
    for (const auto &x : processes) {
        cout << x.ID << "   " << x.exeName << "   " << x.memoryUsage << endl;
    }
}