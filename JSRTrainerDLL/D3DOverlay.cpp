#include "D3DOverlay.hpp"
#include <MinHook.h> // Include MinHook for hooking functions
#include <iostream>
#include <fstream>
#include <Windows.h>

// Original EndScene function pointer
D3DOverlay::EndScene_t D3DOverlay::oEndScene = nullptr;

D3DOverlay::D3DOverlay() : m_pFont(nullptr), m_textRect{ 0, 0, 0, 0 } {
}

D3DOverlay::~D3DOverlay() {
    Cleanup();
}

bool D3DOverlay::Initialize() {
    // Allocate a console for debugging
    AllocConsole();
    FILE* fp;
    if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0) {
        std::cerr << "Failed to redirect stdout to console." << std::endl;
        return false;
    }
    if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0) {
        std::cerr << "Failed to redirect stderr to console." << std::endl;
        return false;
    }
    std::cout << "Console allocated for debugging." << std::endl;

    // Initialize MinHook
    if (MH_Initialize() != MH_OK) {
        std::cerr << "Failed to initialize MinHook." << std::endl;
        return false;
    }

    // Obtain the address of the EndScene function and install the hook
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        std::cerr << "Failed to create Direct3D9 object." << std::endl;
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetForegroundWindow(); // Get the foreground window

    LPDIRECT3DDEVICE9 pDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    if (FAILED(hr)) {
        std::cerr << "Failed to create Direct3D9 device. HRESULT: " << hr << std::endl;
        pD3D->Release();
        return false;
    }

    void** pVTable = *reinterpret_cast<void***>(pDevice);
    oEndScene = reinterpret_cast<EndScene_t>(pVTable[42]); // EndScene is usually the 42nd function in the VTable

    if (MH_CreateHook(reinterpret_cast<void*>(oEndScene), reinterpret_cast<void*>(HookedEndScene), reinterpret_cast<void**>(&oEndScene)) != MH_OK) {
        std::cerr << "Failed to create hook for EndScene." << std::endl;
        pDevice->Release();
        pD3D->Release();
        return false;
    }

    if (MH_EnableHook(reinterpret_cast<void*>(oEndScene)) != MH_OK) {
        std::cerr << "Failed to enable hook for EndScene." << std::endl;
        pDevice->Release();
        pD3D->Release();
        return false;
    }

    pDevice->Release();
    pD3D->Release();

    std::cout << "Successfully hooked EndScene." << std::endl;
    return true;
}

void D3DOverlay::Cleanup() {
    if (m_pFont) {
        m_pFont->Release();
        m_pFont = nullptr;
    }

    MH_DisableHook(reinterpret_cast<void*>(oEndScene));
    MH_RemoveHook(reinterpret_cast<void*>(oEndScene));
    MH_Uninitialize();

    // Free the console
    FreeConsole();
}

HRESULT APIENTRY D3DOverlay::HookedEndScene(LPDIRECT3DDEVICE9 pDevice) {
    if (D3DOverlay* pOverlay = reinterpret_cast<D3DOverlay*>(GetWindowLongPtrA(GetForegroundWindow(), GWLP_USERDATA))) {
        return pOverlay->EndScene(pDevice);
    }
    return oEndScene(pDevice);
}

HRESULT D3DOverlay::EndScene(LPDIRECT3DDEVICE9 pDevice) {
    std::cout << "EndScene called." << std::endl;
    DrawText(pDevice);
    return oEndScene(pDevice);
}

void D3DOverlay::DrawText(LPDIRECT3DDEVICE9 pDevice) {
    if (!m_pFont) {
        HRESULT hr = D3DXCreateFont(pDevice, 24, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &m_pFont);
        if (FAILED(hr)) {
            std::cerr << "Failed to create font. HRESULT: " << hr << std::endl;
            return;
        }

        D3DVIEWPORT9 viewport;
        pDevice->GetViewport(&viewport);
        m_textRect.left = viewport.Width - 200;
        m_textRect.top = viewport.Height - 30;
        m_textRect.right = viewport.Width;
        m_textRect.bottom = viewport.Height;
    }

    if (m_pFont) {
        m_pFont->DrawText(NULL, TEXT("JSRTrainer Active"), -1, &m_textRect, DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
    }
}
