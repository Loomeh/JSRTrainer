#include <Windows.h>
#include "D3DOverlay.hpp"

D3DOverlay overlay;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if (!overlay.Initialize()) {
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        overlay.Cleanup();
        break;
    }
    return TRUE;
}