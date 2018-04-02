#include "winapi_interop.hpp"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>

struct handle_data
{
    unsigned long process_id;
    HWND best_handle;
};

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);
BOOL is_main_window(HWND handle);

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

process_info window_handle_to_process_info(HWND window_handle)
{
    if(window_handle == NULL)
        return process_info();

    TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

    DWORD processID;

    GetWindowThreadProcessId(window_handle, &processID);

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

    process_info info;
    info.process_name = szProcessName;
    info.handle = window_handle;
    info.hProcess = hProcess;
    info.processID = processID;

    RECT rect;
    GetWindowRect(window_handle, &rect);

    info.w = rect.right - rect.left;
    info.h = abs(rect.bottom - rect.top);

    return info;
}

process_info process_id_to_process_info(DWORD processID)
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
        return process_info();
    }

    process_info info;
    info.process_name = szProcessName;
    info.handle = handle;
    info.hProcess = hProcess;
    info.processID = processID;

    RECT rect;
    GetWindowRect(handle, &rect);

    info.w = rect.right - rect.left;
    info.h = abs(rect.bottom - rect.top);

    return info;
}

std::vector<process_info> get_process_infos()
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return std::vector<process_info>();
    }

    cProcesses = cbNeeded / sizeof(DWORD);

    std::vector<process_info> processes;

    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        {
            process_info inf = process_id_to_process_info(aProcesses[i]);

            if(!inf.valid())
                continue;

            processes.push_back(inf);
        }
    }

    return processes;
}
