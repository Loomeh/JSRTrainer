#include <thread>
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <cstring>
#include "memory.hpp"
#include "process.hpp"

// Global constants for hotkeys
const UINT MOD_KEY = MOD_CONTROL;
const int NUM_HOTKEYS = 4;

// Global atomic variables for hotkey state and process running state
std::atomic<bool> hotkeyState(false);
std::atomic<bool> processRunning(true);
std::atomic<bool> timerPaused(false);

// Global variable for keyboard hook
HHOOK hKeyboardHook = NULL;

// Global variables for memory operations
Memory* pMemory = nullptr;
uintptr_t baseAddr = 0;
std::mutex memoryMutex;

// Function prototypes
void PauseTimer();
void RefillHealth();
void SkipLevel();
void FailLevel();

// Low-level keyboard hook function to detect hotkey presses
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        HWND hwndForeground = GetForegroundWindow();
        HWND hwndJetSetRadio = FindWindow(NULL, "Jet Set Radio");

        if (hwndForeground == hwndJetSetRadio && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
            switch (p->vkCode) {
            case '1':
                //std::cout << "Pausing Timer..." << std::endl;
                PauseTimer();
                break;
            case '2':
                std::cout << "Refilling Health..." << std::endl;
                RefillHealth();
                break;
            case '3':
                std::cout << "Skipping Level..." << std::endl;
                SkipLevel();
                break;
            case '4':
                //std::cout << "Failing Level..." << std::endl;
			    FailLevel();
			    break;
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}


// Function to register all hotkeys
// Function to register all hotkeys
bool RegisterAllHotkeys() {
    for (int i = 1; i <= NUM_HOTKEYS; ++i) {
        if (!RegisterHotKey(NULL, i, MOD_KEY, '0' + i)) { // Register hotkey (CTRL + 'i')
            DWORD error = GetLastError();
            if (error == ERROR_HOTKEY_ALREADY_REGISTERED) {
                continue; // Skip already registered hotkeys
            }
            else {
                return false; // Fail to register hotkey
            }
        }
        //std::cout << "Registered hotkey CTRL + " << i << std::endl;
    }
    hotkeyState.store(true);
    return true;
}


// Function to unregister all hotkeys
void UnregisterAllHotkeys() {
    for (int i = 1; i <= NUM_HOTKEYS; ++i) {
        UnregisterHotKey(NULL, i);
        //std::cout << "Unregistered hotkey CTRL + " << i << std::endl;
    }
    hotkeyState.store(false);
}

// Function to manage hotkeys based on the "tabbed" state from memory
void ManageHotkeys() {
    HWND hwndJetSetRadio = FindWindow(NULL, "Jet Set Radio");

    if (hwndJetSetRadio == NULL) {
        std::cerr << "Failed to find Jet Set Radio window." << std::endl;
        return;
    }

    while (processRunning.load()) {
        HWND hwndForeground = GetForegroundWindow();
        bool tabbed = (hwndForeground == hwndJetSetRadio);

        // Register or unregister hotkeys based on the "tabbed" state
        if (tabbed && !hotkeyState.load()) {
            if (!RegisterAllHotkeys()) {
                std::cerr << "Failed to register hotkeys!" << std::endl;
            }
            else {
                //std::cout << "Hotkeys registered." << std::endl;
            }
        }
        else if (!tabbed && hotkeyState.load()) {
            UnregisterAllHotkeys();
            //std::cout << "Hotkeys unregistered." << std::endl;
        }

        // Sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Ensure hotkeys are unregistered on exit
    if (hotkeyState.load()) {
        UnregisterAllHotkeys();
    }
}



// Function to pause the game timer
void PauseTimer() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (pMemory != nullptr) { // Explicitly check against nullptr
        uintptr_t localBaseAddr = baseAddr;
        uintptr_t timerInstAddr = localBaseAddr + 0x000B406B;
		BYTE pauseInstruction[2] = { 0x0F, 0x85 }; 
		BYTE unpauseInstruction[2] = { 0x0F, 0x84 };

        if (timerPaused.load())
        {
            std::cout << "Unpausing timer..." << std::endl;
			pMemory->modifyInstruction(*pMemory, timerInstAddr, unpauseInstruction, sizeof(unpauseInstruction));
			timerPaused.store(false);
        }
        else
        {
			std::cout << "Pausing timer..." << std::endl;
			pMemory->modifyInstruction(*pMemory, timerInstAddr, pauseInstruction, sizeof(pauseInstruction));
			timerPaused.store(true);
        }
    }
}

void RefillHealth() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (pMemory) {
        uintptr_t staticHealthAddress = baseAddr + 0x009FFBC0;
        const DWORD_PTR healthOffsets[] = { 0x58 };

        uintptr_t resolvedHealthAddress = pMemory->ResolveAddress(
            pMemory->GetHandle(),
            staticHealthAddress,
            healthOffsets,
            sizeof(healthOffsets) / sizeof(healthOffsets[0])
        );

        if (resolvedHealthAddress == 0) {
            std::cerr << "Failed to resolve health address." << std::endl;
        }
        else {
            DWORD newValue = 0x9B;
            if (pMemory->WriteToMemory(reinterpret_cast<LPVOID>(resolvedHealthAddress), newValue)) {
                std::cout << "Health value written to: 0x" << std::hex << resolvedHealthAddress << std::dec << std::endl;
            }
            else {
                std::cerr << "Failed to write health value." << std::endl;
            }
        }
    }
}

// Function to skip the current level
void SkipLevel() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (pMemory) {
        uintptr_t localBaseAddr = baseAddr; // Create a local copy of baseAddr
        std::thread([localBaseAddr](Memory* pMemory) {
            uintptr_t skipAddr = localBaseAddr + 0x00038B53;
            uintptr_t syncAddr = localBaseAddr + 0x00038B3B;

            BYTE skipInstruction[2] = { 0x7E, 0x0D }; // Instruction to jump if less or equal
            pMemory->modifyInstruction(*pMemory, skipAddr, skipInstruction, sizeof(skipInstruction));
            pMemory->modifyInstruction(*pMemory, syncAddr, skipInstruction, sizeof(skipInstruction));

            std::cout << "Modified instruction to jle at: 0x" << std::hex << skipAddr << std::dec << std::endl;
            std::cout << "Modified instruction to jle at: 0x" << std::hex << syncAddr << std::dec << std::endl;

            Sleep(500); // Wait for the instruction to take effect

            BYTE restoreInstruction[2] = { 0x7F, 0x0D }; // Instruction to jump if greater
            pMemory->modifyInstruction(*pMemory, skipAddr, restoreInstruction, sizeof(restoreInstruction));
            pMemory->modifyInstruction(*pMemory, syncAddr, restoreInstruction, sizeof(restoreInstruction));

            std::cout << "Restored instruction to jg at: 0x" << std::hex << skipAddr << std::dec << std::endl;
            std::cout << "Restored instruction to jg at: 0x" << std::hex << syncAddr << std::dec << std::endl;
            }, pMemory).detach();
    }
}

void FailLevel()
{
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (pMemory != nullptr)
    {
        uintptr_t localBaseAddr = baseAddr;
        std::thread([localBaseAddr](Memory* pMemory)
        {
            uintptr_t failAddr = localBaseAddr + 0x000B40AB;
            uintptr_t timerInstAddr = localBaseAddr + 0x000B406B;

            BYTE failInstruction[2] = { 0x74, 0x6E };
			BYTE restoreInstruction[2] = { 0x75, 0x6E };
            BYTE timerUnpauseInstruction[2] = { 0x0F, 0x84 };

            if (timerPaused.load())
            {
                std::cout << "Unpausing timer before failing level..." << std::endl;
                pMemory->modifyInstruction(*pMemory, timerInstAddr, timerUnpauseInstruction, sizeof(timerUnpauseInstruction));
                timerPaused.store(false);
            }

            std::cout << "Failing level..." << std::endl;

			pMemory->modifyInstruction(*pMemory, failAddr, failInstruction, sizeof(failInstruction));
			std::cout << "Modified instruction to je at: 0x" << std::hex << failAddr << std::dec << std::endl;

			Sleep(500); // Wait for the instruction to take effect

			pMemory->modifyInstruction(*pMemory, failAddr, restoreInstruction, sizeof(restoreInstruction));
			std::cout << "Restored instruction to jne at: 0x" << std::hex << failAddr << std::dec << std::endl;

		}, pMemory).detach();
    }
}

int main() {
    const char* processName = "jetsetradio.exe";
    Process process;
    DWORD pid = 0;

    std::cout << "Waiting for Jet Set Radio to start..." << std::endl;

    // Wait for the process to start
    while ((pid = process.GetPID(processName)) == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    system("cls");
    std::cout << "Found process ID: " << pid << std::endl;

    // Initialize memory access
    pMemory = new Memory(); // Dynamically allocate Memory
    if (!pMemory->Initialize(pid)) {
        std::cerr << "Failed to initialize memory access." << std::endl;
        delete pMemory;
        return 1;
    }

    MODULEENTRY32 module = process.GetModule(processName, pid);
    if (module.modBaseAddr == nullptr) {
        std::cerr << "Failed to find JSR module." << std::endl;
        delete pMemory;
        return 1;
    }

    baseAddr = reinterpret_cast<uintptr_t>(module.modBaseAddr);

    // Start ManageHotkeys in a separate thread
    std::thread manageHotkeysThread(ManageHotkeys);
    manageHotkeysThread.detach();

    // Install low-level keyboard hook
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (hKeyboardHook == NULL) {
        std::cerr << "Failed to install keyboard hook. Error: " << GetLastError() << std::endl;
        delete pMemory;
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    processRunning.store(false); // Signal threads to stop
    if (hKeyboardHook) UnhookWindowsHookEx(hKeyboardHook);
    delete pMemory; // Clean up the dynamically allocated memory

    return 0;
}
