// memory.hpp

#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <Windows.h>
#include <iostream>

class Memory {
private:
    HANDLE processHandle;
    DWORD processId;
public:
    Memory();
    bool Initialize(DWORD pid);
    HANDLE GetHandle() const;
    ~Memory();

    template<typename T>
    bool ReadFromMemory(LPVOID baseAddress, T* outValue, size_t size = sizeof(T)) {
        if (processHandle == NULL) {
            std::cerr << "Process handle not initialized!" << std::endl;
            return false;
        }

        SIZE_T bytesRead;
        if (!ReadProcessMemory(processHandle, baseAddress, outValue, size, &bytesRead)) {
            std::cerr << "Failed to read memory at 0x" << std::hex << baseAddress
                << ". Error: " << std::dec << GetLastError() << std::endl;
            return false;
        }

#ifdef _DEBUG
        std::cout << "Read operation completed:" << std::endl
            << "Address: 0x" << std::hex << baseAddress << std::endl
            << "Bytes read: " << std::dec << bytesRead << std::endl;
#endif

        return true;
    }

    template<typename T>
    bool WriteToMemory(LPVOID baseAddress, const T& value) {
        if (processHandle == NULL) {
            std::cerr << "Process handle not initialized!" << std::endl;
            return false;
        }

        SIZE_T bytesWritten;
        if (!WriteProcessMemory(processHandle, baseAddress, &value, sizeof(T), &bytesWritten)) {
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

    bool WriteToMemory(LPVOID baseAddress, const BYTE* value, size_t valueSize);
    bool WriteToMemory(LPVOID baseAddress, DWORD newValue);

    uintptr_t ResolveAddress(HANDLE processHandle, uintptr_t baseAddress, const DWORD_PTR* offsets, size_t offsetCount);

    void modifyInstruction(Memory& memory, uintptr_t address, BYTE* instruction, size_t size);
};

#endif // MEMORY_HPP
