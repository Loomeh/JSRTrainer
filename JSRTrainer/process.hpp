#include <Windows.h>
#include <TlHelp32.h>

class Process {
public:
    DWORD GetPID(const char* processName) {
        PROCESSENTRY32 processInfo;
        processInfo.dwSize = sizeof(processInfo);

        HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (processesSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        DWORD pid = 0;
        if (Process32First(processesSnapshot, &processInfo)) {
            do {
                if (_stricmp(processInfo.szExeFile, processName) == 0) {
                    pid = processInfo.th32ProcessID;
                    break;
                }
            } while (Process32Next(processesSnapshot, &processInfo));
        }

        CloseHandle(processesSnapshot);
        return pid;
    }

    MODULEENTRY32 GetModule(const char* moduleName, DWORD processID) {
        MODULEENTRY32 modEntry = { 0 };
        modEntry.dwSize = sizeof(MODULEENTRY32);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return modEntry; // Return empty module entry if snapshot fails
        }

        if (Module32First(hSnapshot, &modEntry)) {
            do {
                if (_stricmp(modEntry.szModule, moduleName) == 0) {
                    CloseHandle(hSnapshot);
                    return modEntry; // Return the found module entry
                }
            } while (Module32Next(hSnapshot, &modEntry));
        }

        CloseHandle(hSnapshot);
        return modEntry; // Return empty module entry if not found
    }
};