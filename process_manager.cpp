#include "process_manager.hpp"
#include "process_info.hpp"
#include "winapi_interop.hpp"
#include <SFML/Graphics.hpp>

process_manager::process_manager()
{
    refresh();
}

std::optional<std::reference_wrapper<application_profile>> process_manager::fetch_profile_by_name(const std::string& name)
{
    for(auto& i :profiles)
    {
        if(i.name == name)
            return i;
    }

    return std::nullopt;
}

void process_manager::toggle_mouse_lock()
{
    lock_mouse_to_window = !lock_mouse_to_window;
}

void process_manager::apply_profile(application_profile& prof, process_info& proc, bool force)
{
    if(prof.applied && !force)
        return;

    if(!prof.enabled && !force)
        return;

    prof.applied = true;

    if(prof.auto_borderless)
    {
        set_borderless(proc, prof.should_move_application, prof.application_x, prof.application_y);
    }

    if(!prof.auto_borderless && prof.should_move_application)
    {
        proc.refresh(true, prof.application_x, prof.application_y);
    }

    Sleep(100);

    ///this doesn't work when we also set the window borderless
    ///why not?
    ///might be because i'm testing on my own window, and it doesn't redraw
    if(prof.auto_lock_mouse)
    {
        proc.lock_mouse_to();

        lock_mouse_to_window = true;

        printf("Confine Mouse\n");
    }

    printf("applied profile\n");
}

void process_manager::refresh()
{
    cleanup();

    processes = get_process_infos();

    auto desk_dim = sf::VideoMode::getDesktopMode();

    dwidth = desk_dim.width;
    dheight = desk_dim.height;

    ///we need to go through all the profiles afterwards and set applied to false for any
    ///processes that don't exist, so that we'll apply this to a process on a relaunch
    for(process_info& proc : processes)
    {
        std::optional opt_profile = fetch_profile_by_name(proc.process_name);

        if(!opt_profile.has_value())
            continue;

        std::cout << "hello prof " << proc.process_name << std::endl;

        auto ref_profile = *opt_profile;

        apply_profile(ref_profile.get(), proc);
    }
}

void process_manager::check_apply_profile_to_foreground_window(double dt_s)
{
    HWND handle = GetForegroundWindow();

    process_info proc = window_handle_to_process_info(handle);

    if(!proc.valid())
    {
        for(application_profile& prof : profiles)
        {
            prof.applied = false;
        }

        return;
    }

    printf("checking\n");

    for(application_profile& prof : profiles)
    {
        if(prof.name == proc.process_name)
        {
            if(prof.applied)
                continue;

            std::cout << "Not applied to " << prof.name << std::endl;

            prof.time_since_detected += dt_s;

            if(prof.time_since_detected < prof.init_delay_s)
                continue;

            std::cout << "applying" << std::endl;

            apply_profile(prof, proc);
        }

        if(prof.name != proc.process_name)
        {
            prof.applied = false;
        }
    }
}

process_info process_manager::fetch_by_name(const std::string& name)
{
    for(auto& i : processes)
    {
        if(i.process_name == name)
            return i;
    }

    return process_info();
}

void process_manager::dump()
{
    for(process_info& inf : processes)
    {
        printf("%s %lu %li\n", inf.process_name.c_str(), inf.handle, inf.hProcess);
    }
}

void process_manager::set_borderless(const process_info& info, bool should_move, int move_w, int move_h)
{
    if(!info.valid())
    {
        printf("Invalid window borderless\n");
        return;
    }

    info.dump_styles();

    auto original_style = info.get_style();
    original_style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    info.set_style(original_style);

    auto original_ex_style = info.get_ex_style();
    original_ex_style &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    info.set_ex_style(original_ex_style);

    info.refresh(should_move, move_w, move_h);

    info.dump_styles();
}

void process_manager::set_bordered(const process_info& info)
{
    if(!info.valid())
    {
        printf("Invalid window bordered\n");
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

bool process_manager::is_windowed(const process_info& info)
{
    if(!info.valid())
    {
        printf("Invalid window windowed\n");
        return false;
    }

    ///hack in lieu of something better
    if(info.process_name == "explorer.exe" || info.process_name == "explorer.EXE")
        return false;

    auto style = info.get_style();

    return ((style & WS_CAPTION) > 0);
}

void process_manager::handle_mouse_lock()
{
    ///below logic can be used to lock the mouse to a specific application
    ///currently using global locking until we have profiles
    /*process_info info = fetch_by_name(last_managed_window);

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

void process_manager::draw_window(int& found_w)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("Applications", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    ///yeah this is pretty crap
    ///but ImGui is a C API so
    std::vector<const char*> names;

    for(auto& i : processes)
    {
        if(!is_windowed(fetch_by_name(i.process_name)) && only_show_windowed && !fetch_profile_by_name(i.process_name).has_value())
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

    ImGui::Checkbox("Refresh on f9", &use_f9_refresh);

    if(names.size() > 0)
    {
        if(ImGui::Button("Make Borderless"))
        {
            set_borderless(fetch_by_name(names[imgui_current_item]), false);

            last_managed_window = names[imgui_current_item];
        }

        if(ImGui::Button("Make Windowed"))
        {
            set_bordered(fetch_by_name(names[imgui_current_item]));

            last_managed_window = names[imgui_current_item];
        }

        if(ImGui::Button("Make Borderless, set to top left"))
        {
            set_borderless(fetch_by_name(names[imgui_current_item]), true);

            last_managed_window = names[imgui_current_item];
        }

        if(ImGui::Button("Make Borderless Auto"))
        {
            process_info info = fetch_by_name(names[imgui_current_item]);

            int a1_diff = abs(info.w - dwidth);
            int a2_diff = abs(info.h - dheight);

            bool move_to_tl = a1_diff < 30 && a2_diff < 30;

            std::cout << "Move? " << move_to_tl << std::endl;

            set_borderless(fetch_by_name(names[imgui_current_item]), move_to_tl);

            last_managed_window = names[imgui_current_item];

            /*process_info new_info = process_id_to_process_info(info.processID);
            bool move_to_tl = new_info.w == dwidth && new_info.h == dheight;
            printf("%i %i %i %i\n", new_info.w, dwidth, new_info.h, dheight);*/
        }

        if(!fetch_profile_by_name(names[imgui_current_item]).has_value() && ImGui::Button("Create Profile"))
        {
            application_profile prof;
            prof.name = names[imgui_current_item];

            profiles.push_back(prof);
        }

        if(fetch_profile_by_name(names[imgui_current_item]).has_value() && ImGui::Button("Delete Profile"))
        {
            for(int i=0; i < (int)profiles.size(); i++)
            {
                if(profiles[i].name == names[imgui_current_item])
                {
                    profiles.erase(profiles.begin() + i);
                    i--;
                    continue;
                }
            }
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

    int window_w = ImGui::GetWindowWidth();

    found_w += window_w;

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(window_w, 0));
    ImGui::Begin("Profiles", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    bool success = false;

    if(names.size() > 0)
    {
        std::optional opt_profile = fetch_profile_by_name(names[imgui_current_item]);

        if(opt_profile.has_value())
        {
            auto ref_profile = *opt_profile;

            ref_profile.get().draw_window_internals();

            if(ref_profile.get().should_apply_immediately)
            {
                auto proc_info = fetch_by_name(ref_profile.get().name);

                apply_profile(ref_profile.get(), proc_info, true);
            }

            success = true;
        }
    }

    if(!success)
    {
        ///yup
        ImGui::Text("        ");
    }

    found_w += ImGui::GetWindowWidth();

    ImGui::End();
}

void process_manager::cleanup()
{
    for(process_info& info : processes)
    {
        CloseHandle(info.handle);
        CloseHandle(info.hProcess);
    }
}

process_manager::~process_manager()
{
    cleanup();
}
