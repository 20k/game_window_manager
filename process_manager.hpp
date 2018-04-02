#ifndef PROCESS_MANAGER_HPP_INCLUDED
#define PROCESS_MANAGER_HPP_INCLUDED

#include <string>
#include <funserialisation/serialise.hpp>
#include "application_profile.hpp"
#include <functional>

struct process_info;

struct process_manager : serialisable
{
    std::string last_managed_window = "";

    std::vector<application_profile> profiles;

    std::vector<process_info> processes;
    int imgui_current_item = 0;

    bool should_quit = false;
    bool only_show_windowed = true;
    bool use_mouse_lock = true;
    bool use_f9_refresh = true;

    bool lock_mouse_to_window = false;

    bool locking_success = false;

    int dwidth = 0;
    int dheight = 0;

    virtual void do_serialise(serialise& s, bool ser) override
    {
        s.handle_serialise(only_show_windowed, ser);
        s.handle_serialise(use_mouse_lock, ser);
        s.handle_serialise(profiles, ser);
    }

    process_manager();

    std::optional<std::reference_wrapper<application_profile>> fetch_profile_by_name(const std::string& name);

    void toggle_mouse_lock();

    void apply_profile(application_profile& prof, process_info& proc, bool force = false);

    void refresh();
    void check_apply_profile_to_foreground_window();

    process_info fetch_by_name(const std::string& name);

    void dump();

    void set_borderless(const std::string& name, bool should_move, int move_w = 0, int move_h = 0);
    void set_bordered(const std::string& name);
    bool is_windowed(const std::string& name);

    void handle_mouse_lock();

    void draw_window(int& found_w);

    void cleanup();
    ~process_manager();
};


#endif // PROCESS_MANAGER_HPP_INCLUDED
