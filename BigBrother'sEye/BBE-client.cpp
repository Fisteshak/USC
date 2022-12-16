#include "windows.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <tchar.h>
#include <tlhelp32.h>
#include <psapi.h>


using namespace std;

struct process
{
    string exeName;
    int ID;
};

vector<process> processes;
vector<process> processes2;

void PrintProcessNameAndID(DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                                      PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
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
            GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

        }
    }

    // Print the process name and identifier.


    _tprintf(TEXT("%s  (PID: %u)  %u\n"), szProcessName, processID, pmc.WorkingSetSize / 1024);

    // Release the handle to the process.

    CloseHandle(hProcess);
}

void getProcessesPSAPI()
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        return;
    }

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for (i = 0; i < cProcesses; i++)
    {
        if (aProcesses[i] != 0)
        {
            PrintProcessNameAndID(aProcesses[i]);
        }
    }
}

int getProcesses(/*vector<process> &processes*/)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    // сделать снимок системы
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return -1;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    process empty;

    // получить первый процесс
    if (!Process32First(hProcessSnap, &pe32))
    {
        cout << "Process32First Error" << endl;
        CloseHandle(hProcessSnap); // clean the snapshot object
        return -1;
    }
    int i = 0;
    int size = 0;
    do
    {
        printf("pe32.th32ProcessID= %d\n", pe32.th32ProcessID);
        printf("pe32.th32ParentProcessID= %d\n", pe32.th32ParentProcessID);
        printf("pe32.szExeFile= %s\n\n", pe32.szExeFile);

        // if (processes.size() <= i) {
        //     processes.push_back(empty);
        // }
        // processes[i].exeName = pe32.szExeFile;
        // processes[i].ID = pe32.th32ProcessID;
        // size += processes[i].exeName.size() + 4;
        // i++;
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return size;
}

int main()
{
    std::string name;

    getProcessesPSAPI();
}