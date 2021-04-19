#include "windows.h"
#include "psapi.h"
#include "processthreadsapi.h"
#include "tlhelp32.h"
#include "string"
#include <comdef.h>
#include <iostream>
#include <fstream>


float Get_RAM(HANDLE hProcess, PROCESSENTRY32 process)
{
     PROCESS_MEMORY_COUNTERS_EX pmc;
    bool flag = GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    float mem = physMemUsedByMe;
    printf("mem = %f KB", mem / 1000);
    printf("\n");
    return mem;
}

float getCurrentCPUValue(HANDLE hProcess, ULARGE_INTEGER lastCPU, ULARGE_INTEGER lastSysCPU, ULARGE_INTEGER lastUserCPU, int numProcessors) {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    float percent;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    percent = (sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    return percent * 100;
}









int main() 
{
    static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
    static int numProcessors;

    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));
   


     // Create toolhelp snapshot.
    PROCESSENTRY32 process;
    process.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    float mem = 0;

    if (Process32First(snapshot, &process) == TRUE)
    {
        while (Process32Next(snapshot, &process) == TRUE)
        {
            _bstr_t b(process.szExeFile);
            const char* c = b;
            
            
            
            if (_stricmp(c, "VVS.exe") == 0)
            {   printf(c);
                
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.th32ProcessID);

                PROCESS_MEMORY_COUNTERS_EX pmc;
                bool flag = GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
                SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
                mem += physMemUsedByMe;
                printf("mem = %f KB", mem/1000);
                printf("\n");
                GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
                memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
                memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));

                CloseHandle(hProcess);
            }
        }
    }

    CloseHandle(snapshot);


    bool flag = true;
    std::ofstream csvfile;
    csvfile.open(".\\monitor\\CPU_RAM.csv", std::fstream::out | std::fstream::trunc);
    csvfile << "time,RAM,CPU \n";
    int time = 0;

    while (flag)
    {
        PROCESSENTRY32 process;
        process.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        float mem = 0;
        bool data = false;
        if (Process32First(snapshot, &process) == TRUE)
        {
            while (Process32Next(snapshot, &process) == TRUE)
            {
                _bstr_t b(process.szExeFile);
                const char* c = b;



                if (_stricmp(c, "VVS.exe") == 0)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.th32ProcessID);

                    float mem = Get_RAM(hProcess, process);

                    float CPU = getCurrentCPUValue(hProcess, lastCPU, lastSysCPU, lastUserCPU, numProcessors);
                    if (mem/1000 < 20000) continue;
    
                    csvfile << time << "," << (mem/1000) << "," << CPU << "\n";
                    Sleep(999);
                    time += 1;
                    data = true;
                }

            }
        }
        if (!data) flag = false;
    }
    csvfile.close();
}