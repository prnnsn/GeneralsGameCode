// SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>

template<class T> struct ReleaseCom { void operator()(T* p) const { if (p) p->Release(); } };
template<class T> using Com = std::unique_ptr<T, ReleaseCom<T>>;

static bool Check(HRESULT hr, const char* operation)
{
    if (SUCCEEDED(hr)) return true;
    std::fprintf(stderr, "%s failed: 0x%08lx\n", operation, static_cast<unsigned long>(hr));
    return false;
}

struct Smoke
{
    Com<ID3D11Device> device;
    Com<ID3D11DeviceContext> context;
    Com<IDXGISwapChain> swap;
    Com<ID3D11RenderTargetView> rtv;
    Com<ID3D11DepthStencilView> dsv;
    UINT width = 0, height = 0;

    ~Smoke() { if (context) { context->ClearState(); context->Flush(); } }

    bool Init(HWND window, bool warp, bool debug)
    {
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferDesc.Width = 320;
        desc.BufferDesc.Height = 240;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 1;
        desc.OutputWindow = window;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL selected = {};
        ID3D11Device* d = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        IDXGISwapChain* sc = nullptr;
        auto create = [&](UINT flags) {
            HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                warp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE, nullptr,
                flags, levels, 1, D3D11_SDK_VERSION, &desc, &sc, &d, &selected, &ctx);
            device.reset(d); context.reset(ctx); swap.reset(sc);
            d = nullptr; ctx = nullptr; sc = nullptr;
            return hr;
        };
        HRESULT hr = create(debug ? D3D11_CREATE_DEVICE_DEBUG : 0);
        if (debug && hr == DXGI_ERROR_SDK_COMPONENT_MISSING)
        {
            std::puts("Debug layer unavailable; retry without it");
            swap.reset(); context.reset(); device.reset();
            hr = create(0);
        }
        if (!Check(hr, "CreateDeviceAndSwapChain") || !device || !context || !swap) return false;
        std::printf("Driver: %s; feature level: 0x%x\n", warp ? "WARP" : "hardware", unsigned(selected));
        IDXGIDevice* dxgi_raw = nullptr;
        if (!Check(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_raw)), "Query IDXGIDevice")) return false;
        Com<IDXGIDevice> dxgi(dxgi_raw);
        IDXGIAdapter* adapter_raw = nullptr;
        if (!Check(dxgi->GetAdapter(&adapter_raw), "GetAdapter")) return false;
        Com<IDXGIAdapter> adapter(adapter_raw);
        DXGI_ADAPTER_DESC ad = {};
        if (!Check(adapter->GetDesc(&ad), "GetAdapterDesc")) return false;
        std::printf("Adapter vendor 0x%x device 0x%x\n", ad.VendorId, ad.DeviceId);
        UINT support = 0;
        if (!Check(device->CheckFormatSupport(DXGI_FORMAT_D24_UNORM_S8_UINT, &support), "Check depth format") ||
            !(support & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)) return false;
        return CreateViews();
    }

    bool CreateViews()
    {
        ID3D11Texture2D* raw = nullptr;
        if (!Check(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&raw)), "GetBuffer")) return false;
        Com<ID3D11Texture2D> back(raw);
        D3D11_TEXTURE2D_DESC bd = {};
        back->GetDesc(&bd);
        ID3D11RenderTargetView* color = nullptr;
        if (!Check(device->CreateRenderTargetView(back.get(), nullptr, &color), "CreateRTV")) return false;
        Com<ID3D11RenderTargetView> next_rtv(color);
        D3D11_TEXTURE2D_DESC dd = {};
        dd.Width = bd.Width; dd.Height = bd.Height;
        dd.MipLevels = 1; dd.ArraySize = 1;
        dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dd.SampleDesc.Count = 1; dd.Usage = D3D11_USAGE_DEFAULT;
        dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        ID3D11Texture2D* depth = nullptr;
        if (!Check(device->CreateTexture2D(&dd, nullptr, &depth), "Create depth texture")) return false;
        Com<ID3D11Texture2D> depth_owner(depth);
        ID3D11DepthStencilView* view = nullptr;
        if (!Check(device->CreateDepthStencilView(depth, nullptr, &view), "CreateDSV")) return false;
        rtv = std::move(next_rtv); dsv.reset(view);
        width = bd.Width; height = bd.Height;
        return true;
    }

    bool Resize(UINT w, UINT h)
    {
        if (!w || !h) return true; // minimized: keep existing resources, no present
        context->OMSetRenderTargets(0, nullptr, nullptr);
        context->ClearState();
        rtv.reset(); dsv.reset();
        context->Flush();
        if (!Check(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0), "ResizeBuffers")) return false;
        return CreateViews();
    }

    bool DrawAndVerify()
    {
        if (!rtv || !dsv) return false;
        ID3D11RenderTargetView* target = rtv.get();
        context->OMSetRenderTargets(1, &target, dsv.get());
        D3D11_VIEWPORT vp = {};
        vp.Width = float(width); vp.Height = float(height); vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);
        const float color[] = {1, 0, 0, 1};
        context->ClearRenderTargetView(rtv.get(), color);
        context->ClearDepthStencilView(dsv.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ID3D11Texture2D* raw = nullptr;
        if (!Check(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&raw)), "Readback buffer")) return false;
        Com<ID3D11Texture2D> back(raw);
        D3D11_TEXTURE2D_DESC rd = {};
        back->GetDesc(&rd);
        rd.Usage = D3D11_USAGE_STAGING; rd.BindFlags = 0; rd.MiscFlags = 0;
        rd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D* staging = nullptr;
        if (!Check(device->CreateTexture2D(&rd, nullptr, &staging), "Create readback")) return false;
        Com<ID3D11Texture2D> read(staging);
        context->OMSetRenderTargets(0, nullptr, nullptr);
        context->CopyResource(read.get(), back.get());
        D3D11_MAPPED_SUBRESOURCE map = {};
        if (!Check(context->Map(read.get(), 0, D3D11_MAP_READ, 0, &map), "Map readback")) return false;
        const unsigned char expected[] = {255, 0, 0, 255};
        bool ok = map.pData && map.RowPitch >= width * 4u;
        if (ok)
        {
            for (UINT y = 0; y < height && ok; ++y)
            {
                const auto* row = static_cast<const unsigned char*>(map.pData) + size_t(y)*map.RowPitch;
                for (UINT x = 0; x < width; ++x)
                    if (std::memcmp(row + x*4u, expected, 4) != 0) { ok = false; break; }
            }
        }
        context->Unmap(read.get(), 0);
        if (!ok) { std::fputs("Readback mismatch\n", stderr); return false; }
        HRESULT hr = swap->Present(0, 0);
        if (FAILED(hr)) std::fprintf(stderr, "Device reason: 0x%08lx\n", static_cast<unsigned long>(device->GetDeviceRemovedReason()));
        if (hr == DXGI_STATUS_OCCLUDED) std::puts("Present occluded (readback verified)");
        return Check(hr, "Present");
    }
};

static LRESULT CALLBACK WindowProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    return DefWindowProcW(w,m,wp,lp);
}

static bool PumpMessages()
{
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

static bool ResizeClient(HWND window, Smoke& smoke, UINT w, UINT h)
{
    RECT rect = {0,0,static_cast<LONG>(w),static_cast<LONG>(h)};
    if (!AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE) ||
        !SetWindowPos(window,nullptr,0,0,rect.right-rect.left,rect.bottom-rect.top,
                      SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) ||
        !PumpMessages() || !GetClientRect(window,&rect))
        return false;
    return rect.right > 0 && rect.bottom > 0 &&
           smoke.Resize(static_cast<UINT>(rect.right),static_cast<UINT>(rect.bottom));
}

static bool MinimizeRestore(HWND window, Smoke& smoke)
{
    ShowWindow(window,SW_MINIMIZE);
    if (!PumpMessages() || !IsIconic(window) || !smoke.Resize(0,0))
        return false;
    ShowWindow(window,SW_RESTORE);
    if (!PumpMessages() || IsIconic(window))
        return false;
    RECT rect = {};
    return GetClientRect(window,&rect) && rect.right > 0 && rect.bottom > 0 &&
           smoke.Resize(static_cast<UINT>(rect.right),static_cast<UINT>(rect.bottom));
}

int main(int argc, char** argv)
{
    bool warp = false, debug = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!std::strcmp(argv[i], "--warp")) warp = true;
        else if (!std::strcmp(argv[i], "--debug")) debug = true;
        else { std::fprintf(stderr, "Unknown argument: %s\n", argv[i]); return 2; }
    }
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = instance;
    wc.lpszClassName = L"GeneralsDX11Smoke";
    if (!RegisterClassW(&wc)) return 1;
    HWND window = CreateWindowW(wc.lpszClassName, L"DX11 smoke test", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, instance, nullptr);
    if (!window) { UnregisterClassW(wc.lpszClassName, instance); return 1; }
    ShowWindow(window, SW_SHOW); UpdateWindow(window);
    bool ok = true;
    for (int pass = 0; pass < 2 && ok; ++pass)
    {
        Smoke test;
        ok = test.Init(window, warp, debug) &&
             ResizeClient(window,test,320,240) && test.DrawAndVerify() &&
             MinimizeRestore(window,test) && test.DrawAndVerify() &&
             ResizeClient(window,test,512,256) && test.DrawAndVerify() &&
             ResizeClient(window,test,320,240) && test.DrawAndVerify();
    }
    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, instance);
    std::puts(ok ? "DX11 smoke passed" : "DX11 smoke failed");
    return ok ? 0 : 1;
}
