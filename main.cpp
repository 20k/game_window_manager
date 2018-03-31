#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <bitset>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui-SFML.h>

struct proc_info
{
    std::string process_name = "Error";
    HWND handle = 0;
    HANDLE hProcess = 0;
    WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

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

    void refresh(bool should_move = false)
    {
        if(!should_move)
            SetWindowPos(handle, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
        else
            SetWindowPos(handle, NULL, 0,0,0,0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
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
    int imgui_current_item = 0;

    process_manager()
    {
        processes = get_process_infos();
    }

    void refresh()
    {
        *this = process_manager();
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

    void set_borderless(const std::string& name, bool should_move)
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

        info.refresh(should_move);

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

    void set_fullscreen(const std::string& name, bool state)
    {
        proc_info info = fetch_by_name(name);

        if(!info.valid())
        {
            printf("Invalid window\n");
            return;
        }

        auto style = info.get_style();

        if(state && (style & WS_OVERLAPPEDWINDOW))
        {
            MONITORINFO mi = { sizeof(mi) };
            if (GetWindowPlacement(info.handle, &info.g_wpPrev) &&
                GetMonitorInfo(MonitorFromWindow(info.handle,
                               MONITOR_DEFAULTTOPRIMARY), &mi)) {
              SetWindowLong(info.handle, GWL_STYLE,
                            style & ~WS_OVERLAPPEDWINDOW);
              SetWindowPos(info.handle, HWND_TOP,
                           mi.rcMonitor.left, mi.rcMonitor.top,
                           mi.rcMonitor.right - mi.rcMonitor.left,
                           mi.rcMonitor.bottom - mi.rcMonitor.top,
                           SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        }

        if(!state && !(style & WS_OVERLAPPEDWINDOW))
        {
            SetWindowLong(info.handle, GWL_STYLE,
                  style | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(info.handle, &info.g_wpPrev);
            SetWindowPos(info.handle, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }

    void draw_window()
    {
        ImGui::Begin("Togglefun", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ///yeah this is pretty crap
        ///but ImGui is a C API so
        std::vector<const char*> names;

        for(auto& i : processes)
        {
            names.push_back(i.process_name.c_str());
        }

        if(names.size() > 0)
        {
            ImGui::ListBox("###Window", &imgui_current_item, &names[0], names.size());

            if(ImGui::Button("Make Borderless"))
            {
                set_borderless(processes[imgui_current_item].process_name, false);
            }

            if(ImGui::Button("Make Windowed"))
            {
                set_bordered(processes[imgui_current_item].process_name);
            }

            if(ImGui::Button("Make Borderless and set to top left"))
            {
                set_borderless(processes[imgui_current_item].process_name, true);
            }

            if(ImGui::Button("Make Fullscreen (not recommended)"))
            {
                set_fullscreen(processes[imgui_current_item].process_name, true);
            }

            if(ImGui::Button("Make Not Fullscreen (broken)"))
            {
                set_fullscreen(processes[imgui_current_item].process_name, false);
            }
        }

        if(ImGui::Button("Refresh"))
        {
            refresh();
        }

        ImGui::End();
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
    sf::RenderWindow window;
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    window.create(sf::VideoMode(800, 600),"Wowee", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildWindowRounding = 2;

    process_manager process_manage;

    sf::Clock ui_clock;

    sf::Clock refresh_clock;

    bool focused = true;
    bool going = true;

    while(going)
    {
        sf::Event event;

        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);

            if(event.type == sf::Event::Closed)
                going = false;

            if(event.type == sf::Event::Resized)
            {
                window.setSize({event.size.width, event.size.height});
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
            }

            if(event.type == sf::Event::GainedFocus)
            {
                focused = true;
            }

            if(event.type == sf::Event::LostFocus)
            {
                focused = false;
            }
        }

        if(refresh_clock.getElapsedTime().asSeconds() > 1)
        {
            refresh_clock.restart();

            //process_manage.refresh();
        }

        process_manage.draw_window();

        ImGui::Render();
        window.display();
        window.clear();

        ImGui::SFML::Update(ui_clock.restart());
    }

    return 0;
}
