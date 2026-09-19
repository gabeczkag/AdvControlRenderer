#include "Window.h"

namespace
{
    constexpr wchar_t WindowClassName[] = L"AdvControlRendererWindow";
}

bool Window::Create(HINSTANCE instance, uint32_t width, uint32_t height, const wchar_t* title)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = WindowClassName;

    if (!RegisterClassExW(&wc))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
    }

    RECT rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    handle = CreateWindowExW(
        0,
        WindowClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        this
    );

    if (!handle)
        return false;

    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);
    return true;
}

bool Window::ProcessMessages()
{
    MSG msg{};

    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<Window*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->handle = hwnd;
    }

    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
