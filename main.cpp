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

#include <funserialisation/serialise.hpp>

struct proc_info
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

struct process_manager : serialisable
{
    std::string last_managed_window = "";

    std::vector<proc_info> processes;
    int imgui_current_item = 0;

    bool should_quit = false;
    bool only_show_windowed = true;
    bool use_mouse_lock = true;

    bool lock_mouse_to_window = false;

    bool locking_success = false;

    int dwidth = 0;
    int dheight = 0;

    virtual void do_serialise(serialise& s, bool ser) override
    {
        s.handle_serialise(only_show_windowed, ser);
        s.handle_serialise(use_mouse_lock, ser);
    }

    process_manager()
    {
        processes = get_process_infos();

        auto desk_dim = sf::VideoMode::getDesktopMode();

        dwidth = desk_dim.width;
        dheight = desk_dim.height;
    }

    void toggle_mouse_lock()
    {
        lock_mouse_to_window = !lock_mouse_to_window;
    }

    void refresh()
    {
        cleanup();

        processes = get_process_infos();

        auto desk_dim = sf::VideoMode::getDesktopMode();

        dwidth = desk_dim.width;
        dheight = desk_dim.height;
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

    bool is_windowed(const std::string& name)
    {
        proc_info info = fetch_by_name(name);

        if(!info.valid())
        {
            printf("Invalid window\n");
            return false;
        }

        ///hack in lieu of something better
        if(name == "explorer.exe" || name == "explorer.EXE")
            return false;

        auto style = info.get_style();

        return ((style & WS_CAPTION) > 0);
    }

    void handle_mouse_lock()
    {
        ///below logic can be used to lock the mouse to a specific application
        ///currently using global locking until we have profiles
        /*proc_info info = fetch_by_name(last_managed_window);

        if(!lock_mouse_to_window || last_managed_window == "" || !info.valid())
        {
            ClipCursor(nullptr);
            return;
        }

        RECT wrect;

        GetWindowRect(info.handle, &wrect);

        ClipCursor(&wrect);*/

        if(!lock_mouse_to_window)
        {
            ClipCursor(nullptr);
            return;
        }

        HWND hwnd = GetForegroundWindow();

        RECT wrect;

        GetWindowRect(hwnd, &wrect);

        ClipCursor(&wrect);
    }

    void draw_window()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::Begin("Togglefun", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);


        ///yeah this is pretty crap
        ///but ImGui is a C API so
        std::vector<const char*> names;

        for(auto& i : processes)
        {
            if(!is_windowed(i.process_name) && only_show_windowed)
               continue;

            names.push_back(i.process_name.c_str());
        }

        if(imgui_current_item >= (int)names.size())
            imgui_current_item = ((int)names.size())-1;

        if(imgui_current_item < 0)
            imgui_current_item = 0;

        if(names.size() > 0)
        {
            ImGui::ListBox("###Window", &imgui_current_item, &names[0], names.size());
        }

        ImGui::Checkbox("Only Show Windowed Applications", &only_show_windowed);

        ImGui::Checkbox("Use End to lock mouse to window", &use_mouse_lock);

        if(names.size() > 0)
        {
            if(ImGui::Button("Make Borderless"))
            {
                set_borderless(names[imgui_current_item], false);

                last_managed_window = names[imgui_current_item];
            }

            if(ImGui::Button("Make Windowed"))
            {
                set_bordered(names[imgui_current_item]);

                last_managed_window = names[imgui_current_item];
            }

            if(ImGui::Button("Make Borderless, set to top left (recommended)"))
            {
                set_borderless(names[imgui_current_item], true);

                last_managed_window = names[imgui_current_item];
            }

            if(ImGui::Button("Make Borderless Auto"))
            {
                proc_info info = fetch_by_name(names[imgui_current_item]);

                int a1_diff = abs(info.w - dwidth);
                int a2_diff = abs(info.h - dheight);

                bool move_to_tl = a1_diff < 30 && a2_diff < 30;

                std::cout << "Move? " << move_to_tl << std::endl;

                set_borderless(names[imgui_current_item], move_to_tl);

                last_managed_window = names[imgui_current_item];

                /*proc_info new_info = process_id_to_proc_info(info.processID);
                bool move_to_tl = new_info.w == dwidth && new_info.h == dheight;
                printf("%i %i %i %i\n", new_info.w, dwidth, new_info.h, dheight);*/
            }
        }

        if(ImGui::Button("Refresh"))
        {
            refresh();
        }

        if(ImGui::Button("Quit"))
        {
            should_quit = true;
        }

        ImGui::End();
    }

    void cleanup()
    {
        for(proc_info& info : processes)
        {
            CloseHandle(info.handle);
            CloseHandle(info.hProcess);
        }
    }

    ~process_manager()
    {
        cleanup();
    }
};

int main()
{
    sf::RenderWindow window;
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    window.create(sf::VideoMode(350, 350),"Wowee", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);

    ImGui::NewFrame();

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 2;
    style.WindowRounding = 2;
    style.ChildWindowRounding = 2;

    process_manager process_manage;

    sf::Keyboard key;
    bool toggled_key = false;

    sf::Clock ui_clock;
    sf::Clock refresh_clock;
    sf::Clock save_clock;

    serialise ser;
    ser.load("save_data.bin");

    ser.handle_serialise_no_clear(process_manage, false);

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
                window.setFramerateLimit(60);
            }

            if(event.type == sf::Event::LostFocus)
            {
                focused = false;
                window.setFramerateLimit(5);
            }
        }

        if(refresh_clock.getElapsedTime().asSeconds() > 1)
        {
            refresh_clock.restart();

            //process_manage.refresh();
        }

        if(save_clock.getElapsedTime().asSeconds() > 1)
        {
            save_clock.restart();

            serialise ser;
            ser.handle_serialise(process_manage, true);

            ser.save("save_data.bin");
        }

        if(!key.isKeyPressed(sf::Keyboard::End))
            toggled_key = false;

        if(key.isKeyPressed(sf::Keyboard::End) && !toggled_key && process_manage.use_mouse_lock)
        {
            toggled_key = true;

            process_manage.refresh();
            process_manage.toggle_mouse_lock();
        }

        process_manage.handle_mouse_lock();
        process_manage.draw_window();

        if(process_manage.should_quit)
            going = false;

        ImGui::Render();
        window.display();
        window.clear();

        ImGui::SFML::Update(ui_clock.restart());

        sf::sleep(sf::milliseconds(4));
    }

    serialise ssave;
    ssave.handle_serialise(process_manage, true);
    ssave.save("save_data.bin");

    return 0;
}
