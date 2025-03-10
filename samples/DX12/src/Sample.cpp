// The MIT License(MIT)
//
// Copyright(c) 2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "windowscodecs")

#include <filesystem>
#include <iostream>
#include <tchar.h>
#include <wincodec.h>

#include "AppRenderer.h"
#include "DeviceResources.h"
#include "UIRenderer.h"
#include "Utilities.h"


DeviceResources deviceResources;
UIData uiData;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int argc, char* argv[])
{
    // Resources
    std::string mediaFolder = "media/images/";
    std::string shadersFolder = "NIS/";

    if (!std::filesystem::exists(mediaFolder))
        mediaFolder = "media/images/";
    if (!std::filesystem::exists(mediaFolder))
        mediaFolder = "../../media/images/";
    if (!std::filesystem::exists(mediaFolder))
        return -1;

    // UI settings
    uiData.Files = getFiles(mediaFolder);
    if (uiData.Files.size() == 0)
        throw std::runtime_error("No media files");
    uiData.FileName = uiData.Files[0].filename().string();
    uiData.FilePath = uiData.Files[0];

    // Create Window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"NVIDIA Image Scaling Demo", nullptr };
    ::RegisterClassEx(&wc);
    RECT wr = { 0, 0, 1280, 1080 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, L"NVIDIA Image Scaling DX12 Demo", WS_OVERLAPPEDWINDOW, 0, 0, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, wc.hInstance, nullptr);
    ::SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Initialize DX12
    deviceResources.create(hwnd, 0);

    // Renderers
    UIRenderer uiRenderer(hwnd, deviceResources, uiData);
    std::vector<std::string> shaderPaths{ "NIS/", "../../../NIS/", "../../DX12/src/" };
    AppRenderer appRenderer(deviceResources, uiData, shaderPaths);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    FPS m_fps;

    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            // press 's' to dump png file
            if (msg.message == WM_KEYDOWN && msg.wParam == 'S')
            {
                appRenderer.saveOutput("dump.png");
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        m_fps.update();
        bool update = false;
        if (appRenderer.updateSize())
        {
            deviceResources.resizeRenderTarget(appRenderer.width(), appRenderer.height());
        }
        deviceResources.PopulateCommandList();
        uiRenderer.update(m_fps.fps());
        appRenderer.update();
        appRenderer.render();
        uiRenderer.render();
        deviceResources.Present(uiData.EnableVsync, 0);
    }
    deviceResources.WaitForGPU();
    uiRenderer.cleanUp();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam == VK_F1)
            uiData.ShowSettings = !uiData.ShowSettings;
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}