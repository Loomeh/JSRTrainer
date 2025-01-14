// memory.cpp

#include "memory.hpp"

Memory::Memory() : processHandle(NULL), processId(0) {}

bool Memory::Initialize(DWORD pid) {
    processId = pid;
    processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processId);
    if (processHandle == NULL) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

HANDLE Memory::GetHandle() const {
    return processHandle;
}

Memory::~Memory() {
    if (processHandle != NULL) {
        CloseHandle(processHandle);
        processHandle = NULL;
    }
}

bool Memory::WriteToMemory(LPVOID baseAddress, const BYTE* value, size_t valueSize) {
    if (processHandle == NULL) {
        std::cerr << "Process handle not initialized!" << std::endl;
        return false;
    }

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, baseAddress, value, valueSize, &bytesWritten)) {
        std::cerr << "Failed to write memory at 0x" << std::hex << baseAddress
            << ". Error: " << std::dec << GetLastError() << std::endl;
        return false;
    }

#ifdef _DEBUG
    std::cout << "Write operation completed:" << std::endl
        << "Address: 0x" << std::hex << baseAddress << std::endl
        << "Bytes written: " << std::dec << bytesWritten << std::endl;
#endif

    return true;
}

bool Memory::WriteToMemory(LPVOID baseAddress, DWORD newValue) {
    if (processHandle == NULL) {
        std::cerr << "Process handle not initialized!" << std::endl;
        return false;
    }

    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, baseAddress, &newValue, sizeof(newValue), &bytesWritten)) {
        std::cerr << "Failed to write memory at 0x" << std::hex << baseAddress
            << ". Error: " << std::dec << GetLastError() << std::endl;
        return false;
    }

#ifdef _DEBUG
    std::cout << "Write operation completed:" << std::endl
        << "Address: 0x" << std::hex << baseAddress << std::endl
        << "Bytes written: " << std::dec << bytesWritten << std::endl;
#endif

    return true;
}

uintptr_t Memory::ResolveAddress(HANDLE processHandle, uintptr_t baseAddress, const DWORD_PTR* offsets, size_t offsetCount) {
    uintptr_t currentAddress = baseAddress;

    for (size_t i = 0; i < offsetCount; ++i) {
        SIZE_T bytesRead;
        uintptr_t tempAddress;

        if (!ReadProcessMemory(processHandle, (LPCVOID)currentAddress, &tempAddress, sizeof(tempAddress), &bytesRead)) {
            std::cerr << "Failed to read pointer at 0x" << std::hex << currentAddress
                << ". Error: " << std::dec << GetLastError() << std::endl;
            return 0;
        }

        currentAddress = tempAddress + offsets[i];
    }

    return currentAddress;
}

void Memory::modifyInstruction(Memory& memory, uintptr_t address, BYTE* instruction, size_t size) {
    memory.WriteToMemory(reinterpret_cast<LPVOID>(address), instruction, size);
}
