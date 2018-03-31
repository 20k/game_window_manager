#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>
#include <string>

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

        auto original_style = info.get_style();

        original_style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

        SetWindowLongPtr(info.handle, GWL_STYLE, original_style);

        LONG lExStyle = info.get_ex_style();
        lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
        SetWindowLongPtr(info.handle, GWL_EXSTYLE, lExStyle);

        SetWindowPos(info.handle, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
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
    process_manager process_manage;
    process_manage.dump();

    process_manage.set_borderless("crapmud_client.exe");

    return 0;
}
