#pragma once

#include <windows.h>
#include <cstdint>

class Window
{
public:
    bool Create(HINSTANCE instance, uint32_t width, uint32_t height, const wchar_t* title);
    bool ProcessMessages();
    HWND GetHandle() const { return handle; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND handle = nullptr;
};
