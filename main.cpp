#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <bitset>
#include <iostream>

struct proc_info
{
    std::string process_name = "Error";
    HWND handle = 0;
    HANDLE hProcess = 0;

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

    void refresh()
    {
        SetWindowPos(handle, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    void dump_styles()
    {
        std::bitset<64> b1(get_style());
        std::bitset<64> b2(get_ex_style());

        std::cout << "b1 " << b1 << std::endl;
        std::cout << "b2 " << b2 << std::endl;
    }
};

#include "winapi_interop.hpp"

struct process_manager
{
    std::vector<proc_info> processes;

    process_manager()
    {
        processes = get_process_infos();
    }

    proc_info fetch_by_name(const std::string& name)
    {
        for(auto& i : processes)
        {
            if(i.process_name == name)
                return i;
        }

        return proc_info();
    }

    void dump()
    {
        for(proc_info& inf : processes)
        {
            printf("%s %lu %li\n", inf.process_name.c_str(), inf.handle, inf.hProcess);
        }
    }

    void set_borderless(const std::string& name)
    {
        proc_info info = fetch_by_name(name);

        if(!info.valid())
        {
            printf("Invalid window\n");
            return;
        }

        info.dump_styles();

        auto original_style = info.get_style();
        original_style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

        info.set_style(original_style);

        auto original_ex_style = info.get_ex_style();
        original_ex_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

        info.set_ex_style(original_ex_style);

        info.refresh();

        info.dump_styles();
    }

    void set_bordered(const std::string& name)
    {
        proc_info info = fetch_by_name(name);

        if(!info.valid())
        {
            printf("Invalid window\n");
            return;
        }

        info.dump_styles();

        auto original_style = info.get_style();
        original_style |= (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

        info.set_style(original_style);

        auto original_ex_style = info.get_ex_style();
        original_ex_style |= (WS_EX_DLGMODALFRAME);// | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

        info.set_ex_style(original_ex_style);

        info.refresh();

        info.dump_styles();
    }

    ~process_manager()
    {
        for(proc_info& info : processes)
        {
            CloseHandle(info.handle);
            CloseHandle(info.hProcess);
        }
    }
};

int main()
{
    {
        process_manager process_manage;
        process_manage.dump();

        process_manage.set_borderless("crapmud_client.exe");
    }

    Sleep(1000);

    {
        process_manager process_manage;
        process_manage.set_bordered("crapmud_client.exe");
    }

    return 0;
}
