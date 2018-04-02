#ifndef PROCESS_INFO_HPP_INCLUDED
#define PROCESS_INFO_HPP_INCLUDED

#include <string>
#include <windows.h>
#include <bitset>
#include <iostream>

struct process_info
{
    std::string process_name = "Error";
    HWND handle = 0;
    HANDLE hProcess = 0;
    WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };
    DWORD processID = 0;

    int w = 0;
    int h = 0;

    bool valid()
    {
        return handle != 0;
    }

    LONG_PTR get_style()
    {
        return GetWindowLongPtr(handle, GWL_STYLE);
    }

    LONG_PTR get_ex_style()
    {
        return GetWindowLongPtr(handle, GWL_EXSTYLE);
    }

    void set_style(LONG_PTR dat)
    {
        SetWindowLongPtr(handle, GWL_STYLE, dat);
    }

    void set_ex_style(LONG_PTR dat)
    {
        SetWindowLongPtr(handle, GWL_EXSTYLE, dat);
    }

    void refresh(bool should_move = false, int move_x = 0, int move_y = 0)
    {
        if(!should_move)
            SetWindowPos(handle, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        else
            SetWindowPos(handle, NULL, move_x, move_y, 0,0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    void dump_styles()
    {
        std::bitset<64> b1(get_style());
        std::bitset<64> b2(get_ex_style());

        std::cout << "b1 " << b1 << std::endl;
        std::cout << "b2 " << b2 << std::endl;
    }

    void lock_mouse_to()
    {
        RECT wrect;
        GetWindowRect(handle, &wrect);
        ClipCursor(&wrect);
    }
};

#endif // PROCESS_INFO_HPP_INCLUDED
