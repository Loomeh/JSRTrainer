#ifndef D3DOVERLAY_HPP
#define D3DOVERLAY_HPP

#include <d3d9.h>
#include <d3dx9.h>
#include <Windows.h>

class D3DOverlay {
public:
    D3DOverlay();
    ~D3DOverlay();

    bool Initialize();
    void Cleanup();

private:
    static HRESULT APIENTRY HookedEndScene(LPDIRECT3DDEVICE9 pDevice);
    HRESULT EndScene(LPDIRECT3DDEVICE9 pDevice);
    void DrawText(LPDIRECT3DDEVICE9 pDevice);

    using EndScene_t = HRESULT(APIENTRY*)(LPDIRECT3DDEVICE9);
    static EndScene_t oEndScene;

    ID3DXFont* m_pFont;
    RECT m_textRect;
};

#endif // D3DOVERLAY_HPP