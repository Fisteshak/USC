#include <iostream>
#include "Windows.h"
#include "WtsApi32.h"
int main()
{
    WTS_PROCESS_INFO* pWPIs = NULL;
    DWORD dwProcCount = 0;
    if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount)) {
        for (DWORD i = 0; i < dwProcCount; i++) {
            std::cout << pWPIs[i].pProcessName << std::endl;
        }
    }

    if (pWPIs) {
        WTSFreeMemory(pWPIs);
    }
}