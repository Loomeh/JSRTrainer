#ifndef D3DMANAGER_HPP
#define D3DMANAGER_HPP

#include <d3d9.h>
#include <d3dx9.h>
#include <Windows.h>

class D3DManager {
public:
    D3DManager();
    ~D3DManager();

    bool HookEndScene(HWND hWnd);
    void UnhookEndScene();

private:
    // Hooking function prototype
    static HRESULT APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pDevice);

    HRESULT EndScene(LPDIRECT3DDEVICE9 pDevice);
    void RenderText(LPDIRECT3DDEVICE9 pDevice);

    // Original EndScene function pointer
    using EndScene_t = HRESULT(APIENTRY*)(LPDIRECT3DDEVICE9);
    static EndScene_t oEndScene;

    // Font for rendering text
    ID3DXFont* m_pFont;
    RECT m_textRect;

    // Pointer to the vtable
    void** vTable;
};

#endif // D3DMANAGER_HPP