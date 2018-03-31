#ifndef WINAPI_INTEROP_HPP_INCLUDED
#define WINAPI_INTEROP_HPP_INCLUDED

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);
BOOL is_main_window(HWND handle);

struct handle_data
{
    unsigned long process_id;
    HWND best_handle;
};

HWND find_main_window(unsigned long process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.best_handle = 0;
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.best_handle;
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    handle_data& data = *(handle_data*)lParam;
    unsigned long process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id || !is_main_window(handle))
    {
        return TRUE;
    }
    data.best_handle = handle;
    return FALSE;
}

BOOL is_main_window(HWND handle)
{
    return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

proc_info process_id_to_proc_info(DWORD processID)
{
    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );


    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod),
                                 &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName,
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }

    HWND handle = find_main_window(processID);

    if(handle == 0)
    {
        CloseHandle(hProcess);
        return proc_info();
    }

    proc_info info;
    info.process_name = szProcessName;
    info.handle = handle;
    info.hProcess = hProcess;

    return info;
}

std::vector<proc_info> get_process_infos()
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return std::vector<proc_info>();
    }

    cProcesses = cbNeeded / sizeof(DWORD);

    std::vector<proc_info> processes;

    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        {
            proc_info inf = process_id_to_proc_info(aProcesses[i]);

            if(!inf.valid())
                continue;

            processes.push_back(inf);
        }
    }

    return processes;
}

#endif // WINAPI_INTEROP_HPP_INCLUDED
